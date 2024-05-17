#include "compress.h"
#include <stdlib.h>

void *compress(void *data, size_t inputLen, size_t *outputLen)
{
	// Magic happens here!
	return NULL;
}


/*
void old_code() {
	// TODO: Break up this logic into smaller functions to be used by 'compress()'.

	// Create key data.
	COMPRESSION_KEY *keys;

	for (currentCursor = 1; currentCursor <= outputSize - 3; currentCursor++)
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
					if (currentCursor+matchLength <= outputSize)
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

	// Terminate the key.
	keySize++;
	keys = (COMPRESSION_KEY *)realloc(keys, keySize * sizeof(COMPRESSION_KEY));

	keys[keySize-1].destOffset = -1;
	keys[keySize-1].copyCount = 0;

	// Create key bitfield based on recorded key data.
	keyCursor = 0;
	for (int currentOffset = 0; i < outputSize; i++)
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

	// TODO: Finish me!
	// TODO: Code beyond this point hasn't been fully updated yet.

	// add terminator to key
	bitfield_size++;
	bitfield = (byte *)realloc(bitfield, bitfield_size);
	bitfield[bitfield_size-1] = 0xF;

	// if needed, align the bitfield to the next whole byte (8 bits)
	while (bitfield_size & 7)
	{
		bitfield_size++;
		bitfield = (byte *)realloc(bitfield, bitfield_size);

		bitfield[bitfield_size-1] = 0;
	}

	// write data
	n = 0;	// used as subscript to key[]
	b = 0;	// used as subscript to bitfield[]
	for (i = 0; i < outputSize; i++)
	{
		if (i < outputSize)
		{
			if ((b & 7) == 0)
			{
				data_comp_1 = 0;

				if(bitfield[b] == 0xF)	data_comp_1 |= 1;
				else					data_comp_1 |= bitfield[b];

				if(bitfield[b+1] == 0xF)	data_comp_1 |= 2;
				else					data_comp_1 |= (bitfield[b+1]<<1);

				if(bitfield[b+2] == 0xF)	data_comp_1 |= 4;
				else					data_comp_1 |= (bitfield[b+2]<<2);

				if(bitfield[b+3] == 0xF)	data_comp_1 |= 8;
				else					data_comp_1 |= (bitfield[b+3]<<3);

				if(bitfield[b+4] == 0xF)	data_comp_1 |= 16;
				else					data_comp_1 |= (bitfield[b+4]<<4);

				if(bitfield[b+5] == 0xF)	data_comp_1 |= 32;
				else					data_comp_1 |= (bitfield[b+5]<<5);

				if(bitfield[b+6] == 0xF)	data_comp_1 |= 64;
				else					data_comp_1 |= (bitfield[b+6]<<6);

				if(bitfield[b+7] == 0xF)	data_comp_1 |= 128;
				else					data_comp_1 |= (bitfield[b+7]<<7);

				fputc(data_comp_1, out_file);
				compressed_size++;
			}

			if (data_comp_1 & 1)
			{
				fputc(keys[n+3], out_file);
				fputc(keys[n+2], out_file);
				compressed_size += 2;

				i += (*(unsigned short *)&keys[n+2] & 0xF);
				n += 4;
			}
			else
			{
				fputc(uncompressed[i], out_file);
				compressed_size++;
			}

			data_comp_1 >>= 1;
			b++;
		}
	}

	if ((b&7) == 0)
	{
		if (bitfield[b] == 0xF)
		{
			// let 'i' go one over to allow special cases
			// where terminator needs to be inserted
			// (e.g. MAP14 "SEGS")
			b = 1;
			fputc(b, out_file);
			compressed_size++;
		}
	}

	// add terminator
	compressed_size += 2;
	i = 0;
	fwrite(&i, 2, 1, out_file);

	while (compressed_size & 3){
		fputc(i, out_file);
		compressed_size++;
	}

	WriteTable(lump, out_file_size, outputSize);

	out_file_size += compressed_size;
}
//*/
