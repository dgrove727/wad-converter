#include "common.h"
#include "compress.h"
#include <stdlib.h>

byte *compress(byte *data, size_t inputLen, size_t *outputLen)
{
	COMPRESSION_KEY *keys = CreateKeys(data, inputLen);
	byte *keyBits = (byte *)CreateKeyBits(keys, inputLen);	// TODO: Investigate. Is this actually necessary if we have 'keys'?
	byte *output = (byte *)CreateCompressedData(data, keys, keyBits, inputLen, outputLen);

	free(keys);
	free(keyBits);

	return output;
}


// TODO: Make private
COMPRESSION_KEY *CreateKeys(void *data, int inputLen)
{
	// Create key data.
	COMPRESSION_KEY *keys = (COMPRESSION_KEY *)malloc(1);
	int keySize = 0;

	byte *uncompressed = static_cast<byte *>(data);

	for (int currentCursor = 1; currentCursor <= inputLen - 3; currentCursor++)
	{
		// Collect the next three bytes.
		long nextThreeBytes = (uncompressed[currentCursor]<<16)
		 | (uncompressed[currentCursor+1]<<8)
		 | uncompressed[currentCursor+2];

		int highestMatchLength = 0;
		int matchOffset = 0;
		int matchLength = 0;

		// Look for the byte series behind the current offset.
		for (int previousCursor = 0; previousCursor < currentCursor; previousCursor++)
		{
			// The compression format only allows the previous 4KB to be referenced.
			if (currentCursor-previousCursor > 0x1000) {
				previousCursor = currentCursor - 0x1000;
			}

			// Collect three previous bytes.
			long previousThreeBytes = (uncompressed[previousCursor]<<16)
			 | (uncompressed[previousCursor + (1%(currentCursor-previousCursor))]<<8)
			 | uncompressed[previousCursor + (2%(currentCursor-previousCursor))];

			// If the three bytes found before next three bytes are a match, figure out how many more are a match.
			if (nextThreeBytes == previousThreeBytes)
			{
				if (highestMatchLength == 0)
				{
					// We have three bytes matching up to this point, so go ahead and record that.
					highestMatchLength = 3;
					matchOffset = previousCursor;
				}

				// Search for additional bytes in our match set.
				// The compression format allows a maximum match size of 16.
				for (int matchLength=3; matchLength <= 16; matchLength++)
				{
					if (currentCursor+matchLength <= inputLen)
					{
						// We haven't reached the end of the lump size yet.
						// Continue checking for byte matches.
						if (uncompressed[currentCursor+matchLength] != uncompressed[previousCursor + (matchLength%(currentCursor-previousCursor))])
						{
							// We found a byte mismatch.
							// If this most recent comparison yielded a larger match, overwrite the quantity we recorded previously.
							if(matchLength >= highestMatchLength)
							{
								highestMatchLength = matchLength;
								matchOffset = previousCursor;
							}
							break;
						}
					}
					else
					{
						// We've reached the end of the lump size.
						// If this most recent comparison yielded a larger match, overwrite the quantity we recorded previously.
						if (matchLength >= highestMatchLength)
						{
							highestMatchLength = matchLength-1;
							matchOffset = previousCursor;
						}
						break;
					}
				}

				// If there are more than 16 bytes that match, record a match quantity of 16.
				if (matchLength == 17)
				{
					highestMatchLength = 16;
					matchOffset = previousCursor;
				}
			}
		}

		// If a match was found, create a key for it.
		if (highestMatchLength)
		{
			keySize++;
			keys = (COMPRESSION_KEY *)realloc(keys, keySize * sizeof(COMPRESSION_KEY));

			keys[keySize-1].destOffset = currentCursor;
			keys[keySize-1].srcOffset = matchOffset;
			keys[keySize-1].copyCount = highestMatchLength;

			currentCursor += (highestMatchLength-1);

			highestMatchLength = 0;
		}
	}

	// Add a key to terminate the collection.
	keySize++;
	keys = (COMPRESSION_KEY *)realloc(keys, keySize * sizeof(COMPRESSION_KEY));

	keys[keySize-1].destOffset = -1;
	keys[keySize-1].copyCount = 0;

	return keys;
}


// TODO: Make private
void *CreateKeyBits(COMPRESSION_KEY *keys, int inputLen)
{
	// Create key bitfield based on recorded key data.
	byte *bitfield = (byte *)malloc(1);
	int bitfieldSize = 0;

	int keyCursor = 0;
	for (int currentOffset = 0; currentOffset < inputLen; currentOffset++)
	{
		if(currentOffset < keys[keyCursor].destOffset)
		{
			// This byte will be marked as uncompressed in the bitfield.
			bitfieldSize++;
			bitfield = (byte *)realloc(bitfield, bitfieldSize);

			bitfield[bitfieldSize-1] = 0;
		}
		else
		{
			// This byte is referenced by a key, so mark it as compressed in the bitfield.
			bitfieldSize++;
			bitfield = (byte *)realloc(bitfield, bitfieldSize);

			bitfield[bitfieldSize-1] = 1;

			currentOffset += keys[keyCursor].copyCount;
			keyCursor++;
		}
	}

	// Add a terminator.
	bitfieldSize++;
	bitfield = (byte *)realloc(bitfield, bitfieldSize);
	bitfield[bitfieldSize-1] = 0xF;

	// If needed, align the bitfield to the next whole byte (8 bits)
	while (bitfieldSize & 7)
	{
		bitfieldSize++;
		bitfield = (byte *)realloc(bitfield, bitfieldSize);

		bitfield[bitfieldSize-1] = 0;
	}

	return bitfield;
}


// TODO: Make private
void *CreateCompressedData(void *uncompressed, COMPRESSION_KEY *keys, byte *keyBits, size_t inputLen, size_t *outputLen) {
	// Write data
	int keyCursor = 0;
	int bitCursor = 0;
	int compressedSize = 0;

	byte *compressed = (byte *)malloc((inputLen * 1.125) + 4);	// This should be plenty big for the compressed data. Resize later.

	byte *bitfield = static_cast<byte *>(keyBits);

	for (int inputCursor = 0; inputCursor < (int)inputLen; inputCursor++)
	{
		byte bits = 0;
		if ((bitCursor & 7) == 0)
		{
			// We have no bits to read, so grab the next set of eight.
			// TODO: Investigate. Why check each bit for a terminator, even though only one is ever set in CreateKeyBits()?
			if(bitfield[bitCursor] == 0xF)		bits |= 1;
			else								bits |= bitfield[bitCursor];

			if(bitfield[bitCursor+1] == 0xF)	bits |= 2;
			else								bits |= (bitfield[bitCursor+1]<<1);

			if(bitfield[bitCursor+2] == 0xF)	bits |= 4;
			else								bits |= (bitfield[bitCursor+2]<<2);

			if(bitfield[bitCursor+3] == 0xF)	bits |= 8;
			else								bits |= (bitfield[bitCursor+3]<<3);

			if(bitfield[bitCursor+4] == 0xF)	bits |= 16;
			else								bits |= (bitfield[bitCursor+4]<<4);

			if(bitfield[bitCursor+5] == 0xF)	bits |= 32;
			else								bits |= (bitfield[bitCursor+5]<<5);

			if(bitfield[bitCursor+6] == 0xF)	bits |= 64;
			else								bits |= (bitfield[bitCursor+6]<<6);

			if(bitfield[bitCursor+7] == 0xF)	bits |= 128;
			else								bits |= (bitfield[bitCursor+7]<<7);

			compressed[compressedSize] = bits;
			compressedSize++;
		}

		if (bits & 1)
		{
			// The current bit is 1, meaning the data is compressed.
			// Write the offset and copy count to the output.
			unsigned short packedData = (unsigned short)(((keys[keyCursor].destOffset - keys[keyCursor].srcOffset) << 4) | (keys[keyCursor].copyCount - 1));
			compressed[compressedSize] = packedData >> 8;
			compressed[compressedSize+1] = (byte)packedData;
			compressedSize += 2;

			inputCursor += (keys[keyCursor].copyCount - 1);
			keyCursor += 4;
		}
		else
		{
			// The current bit is 0, meaning the data is uncompressed.
			// Write the byte to the output.
			compressed[compressedSize] = ((byte *)uncompressed)[inputCursor];	// TODO: Does this work? Review the results...
			compressedSize++;
		}

		// Advance to the next bit.
		bits >>= 1;
		bitCursor++;
	}

	if ((bitCursor&7) == 0)
	{
		if (bitfield[bitCursor] == 0xF)
		{
			// Let 'bitCursor' go one over to allow special cases where the
			// terminator needs to be inserted (e.g. MAP14 "SEGS").
			bitCursor = 1;
			compressed[compressedSize] = bitCursor;
			compressedSize++;
		}
	}

	// Add terminator
	compressed[compressedSize] = 0;
	compressed[compressedSize+1] = 0;
	compressedSize += 2;

	// Size the data to the next nearest 4-byte interval.
	while (compressedSize & 3){
		compressed[compressedSize] = 0;
		compressedSize++;
	}

	// Resize the buffer now that we're done writing all the data.
	compressed = (byte *)realloc(compressed, compressedSize);

	*outputLen = compressedSize;

	return compressed;
}
