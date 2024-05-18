#include "common.h"
#include "compress.h"
#include <stdlib.h>

void *compress(void *data, size_t inputLen, size_t *outputLen)
{
	//COMPRESSION_KEY *keys = CreateKeys();
	//byte *keyBits = (byte *)CreateKeyBits(keys);
	//CreateCompressedData(keys);

	return NULL;
}


/*
COMPRESSION_KEY *CreateKeys()
{
	// Create key data.
	COMPRESSION_KEY *keys = (COMPRESSION_KEY *)malloc(1);
	int keySize = 0;

	for (int currentCursor = 1; currentCursor <= outputLen - 3; currentCursor++)
	{
		// Collect the next three bytes.
		nextThreeBytes = (uncompressed[currentCursor]<<16)
		 | (uncompressed[currentCursor+1]<<8)
		 | uncompressed[currentCursor+2];

		// Look for the byte series behind the current offset.
		for (previousCursor = 0; previousCursor < currentCursor; n++)
		{
			// The compression format only allows the previous 4KB to be referenced.
			if (currentCursor-previousCursor > 0x1000) {
				previousCursor = currentCursor - 0x1000;
			}

			// Collect three previous bytes.
			previousThreeBytes = (uncompressed[previousCursor]<<16)
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
					if (currentCursor+matchLength <= outputLen)
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



void *CreateKeyBits(COMPRESSION_KEY *keys)
{
	// Create key bitfield based on recorded key data.
	byte *bitfield = malloc(1);
	int bitfieldSize = 0;

	int keyCursor = 0;
	for (int currentOffset = 0; i < outputLen; i++)
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



void CreateCompressedData(COMPRESSION_KEY *keys) {
	// TODO: Break up this logic into smaller functions to be used by 'compress()'.

	// TODO: Finish me!
	// TODO: Code beyond this point hasn't been fully updated yet.

	// Write data
	int keyCursor = 0;
	int bitCursor = 0;
	for (currentCursor = 0; currentCursor < outputLen; currentCursor++)
	{
		if ((bitCursor & 7) == 0)
		{
			// We have no bits to read, so grab the next set of eight.
			data_comp_1 = 0;

			if(bitfield[bitCursor] == 0xF)		data_comp_1 |= 1;
			else								data_comp_1 |= bitfield[bitCursor];

			if(bitfield[bitCursor+1] == 0xF)	data_comp_1 |= 2;
			else								data_comp_1 |= (bitfield[bitCursor+1]<<1);

			if(bitfield[bitCursor+2] == 0xF)	data_comp_1 |= 4;
			else								data_comp_1 |= (bitfield[bitCursor+2]<<2);

			if(bitfield[bitCursor+3] == 0xF)	data_comp_1 |= 8;
			else								data_comp_1 |= (bitfield[bitCursor+3]<<3);

			if(bitfield[bitCursor+4] == 0xF)	data_comp_1 |= 16;
			else								data_comp_1 |= (bitfield[bitCursor+4]<<4);

			if(bitfield[bitCursor+5] == 0xF)	data_comp_1 |= 32;
			else								data_comp_1 |= (bitfield[bitCursor+5]<<5);

			if(bitfield[bitCursor+6] == 0xF)	data_comp_1 |= 64;
			else								data_comp_1 |= (bitfield[bitCursor+6]<<6);

			if(bitfield[bitCursor+7] == 0xF)	data_comp_1 |= 128;
			else								data_comp_1 |= (bitfield[bitCursor+7]<<7);

			fputc(data_comp_1, out_file);
			compressed_size++;
		}

		if (data_comp_1 & 1)
		{
			// The current bit is 1, meaning the data is compressed.
			// Write the offset and copy count to the output.
			unsigned short packedData = ((keys[keyCursor].destOffset - keys[keyCursor].srcOffset) << 4) | (keys[keyCursor].copyCount - 1)
			fputc(packedData >> 8, out_file);
			fputc(packedData & 0xFF, out_file);
			compressed_size += 2;

			currentCursor += (keys[keyCursor].copyCount - 1);
			keyCursor += 4;
		}
		else
		{
			// The current bit is 0, meaning the data is uncompressed.
			// Write the byte to the output.
			fputc(uncompressed[currentCursor], out_file);
			compressed_size++;
		}

		// Advance to the next bit.
		data_comp_1 >>= 1;
		bitCursor++;
	}

	if ((bitCursor&7) == 0)
	{
		if (bitfield[bitCursor] == 0xF)
		{
			// let 'i' go one over to allow special cases
			// where terminator needs to be inserted
			// (e.g. MAP14 "SEGS")
			bitCursor = 1;
			fputc(bitCursor, out_file);
			compressed_size++;
		}
	}

	// add terminator
	compressed_size += 2;
	currentCursor = 0;
	fwrite(&currentCursor, 2, 1, out_file);

	while (compressed_size & 3){
		fputc(currentCursor, out_file);
		compressed_size++;
	}

	WriteTable(lump, out_file_size, outputLen);

	out_file_size += compressed_size;
}
//*/
