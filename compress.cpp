#include "compress.h"
#include <stdlib.h>

void *compress(void *data, size_t inputLen, size_t *outputLen)
{
	// Magic happens here!
	return NULL;
}


// TODO: Continue cleaning up the code below.
/*
void old_code() {
	// Create key data.
	for (i = 1; i <= outputSize - 3; i++)
	{
		// Collect the next three bytes.
		nextThreeBytes = (uncompressed[i]<<16)
		 | (uncompressed[i+1]<<8)
		 | uncompressed[i+2];

		// Look for the byte series behind the current offset.
		for (n = 0; n < i; n++)
		{
			// The compression format only allows the previous 4KB to be referenced.
			if (i-n > 0x1000) {
				n = i - 0x1000;
			}

			// Collect three previous bytes.
			previousThreeBytes = (uncompressed[n]<<16)
			 | (uncompressed[n + (1%(i-n))]<<8)
			 | uncompressed[n + (2%(i-n))];

			// If the three bytes found before next three bytes are a match, figure out how many more are a match.
			if (nextThreeBytes == previousThreeBytes)
			{
				if (highestMatchLength == 0)
				{
					// We have three bytes matching up to this point, so go ahead and record that.
					highestMatchLength = 3;
					b = n;
				}

				// Search for additional bytes in our match set.
				// The compression format allows a maximum match size of 16.
				for (int matchLength=3; matchLength <= 16; matchLength++)
				{
					if (i+matchLength <= outputSize)
					{
						// We haven't reached the end of the lump size yet.
						// Continue checking for byte matches.
						if (uncompressed[i+matchLength] != uncompressed[n + (matchLength%(i-n))])
						{
							// We found a byte mismatch.
							// If this most recent comparison yielded a larger match, overwrite the quantity we recorded previously.
							if(matchLength >= highestMatchLength)
							{
								highestMatchLength = matchLength;
								b = n;		// save 'n' for referencing later
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
							b = n;		// save 'n' for referencing later
						}
						break;
					}
				}

				// If there are more than 16 bytes that match, record a match quantity of 16.
				if (matchLength == 17)
				{
					highestMatchLength = 16;
					b = n;
				}
			}
		}

		// If a match was found, record it in the key.
		if (highestMatchLength)
		{
			key_size += 4;
			key = (byte *)realloc(key, key_size);

			*(unsigned short *)&key[key_size - 4] = i;
			*(unsigned short *)&key[key_size - 2] = (((i-b)-1)<<4) | (highestMatchLength-1);

			i += (highestMatchLength-1);

			highestMatchLength = 0;
		}
	}

	// Terminate the key.
	keySize += 4;
	key = (byte *)realloc(key, keySize);

	*(unsigned short *)&key[keySize - 4] = 0xFFFF;
	*(unsigned short *)&key[keySize - 2] = 0;

	// TODO: Add more commentary beyond this point to give the remaining code more clarity.
	// TODO: Rename some of these variables.

	// Create key bitfield
	n = 0;
	for (i = 0; i < outputSize; i++)
	{
		if(i < *(unsigned short *)&key[n])
		{
			bitfield_size++;
			bitfield = (byte *)realloc(bitfield, bitfield_size);

			bitfield[bitfield_size-1] = 0;
		}
		else
		{
			bitfield_size++;
			bitfield = (byte *)realloc(bitfield, bitfield_size);

			bitfield[bitfield_size-1] = 1;

			i += (*(unsigned short *)&key[n+2] & 0xF);
			n += 4;
		}
	}

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
				fputc(key[n+3], out_file);
				fputc(key[n+2], out_file);
				compressed_size += 2;

				i += (*(unsigned short *)&key[n+2] & 0xF);
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
*/
