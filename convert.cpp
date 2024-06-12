#include "convert.h"
#include <stdlib.h>
#include <memory.h>

#define READBYTE(p)   *((byte *)p)
#define READCHAR(p)   *((char *)p)
#define READSHORT(p)  *((short *)p)
#define READUSHORT(p) *((unsigned short *)p)
#define READINT(p)    *((int *)p)
#define READUINT(p)   *((unsigned int *)p)

#define WRITEBYTE(p,b)    { byte *p_tmp = (byte *)p; *p_tmp = (byte)(b); p_tmp++; p = (byte *)p_tmp; }
#define WRITECHAR(p,b)    { char *p_tmp = (char *)p; *p_tmp = (char)(b); p_tmp++; p = (byte *)p_tmp; }
#define WRITESHORT(p,b)   { short *p_tmp = (short *)p; *p_tmp = (short)(b); p_tmp++; p = (byte *)p_tmp; }
#define WRITEUSHORT(p,b)  { unsigned short *p_tmp = (unsigned short *)p; *p_tmp = (unsigned short)(b); p_tmp++; p = (byte *)p_tmp; }
#define WRITEINT(p,b)     { int *p_tmp = (int *)p; *p_tmp = (int)(b); p_tmp++; p = (byte *)p_tmp; }
#define WRITEUINT(p,b)    { unsigned int *p_tmp = (unsigned int *)p; *p_tmp = (unsigned int)(b); p_tmp++; p = (byte *)p_tmp; }

// TODO: We also need to convert UI graphics to that Jaguar format, as well
byte *ConvertUIGraphicFromPCToJag(byte *lumpData, int lumpSize, int *jagDataLen)
{
	return NULL;
}

//
// Allocate sufficient space in 'jagHeader' and 'jagData' before calling.
//
void ConvertSpriteDataFromPCToJag(byte *lumpData, int lumpSize, byte *jagHeader, int *jagHeaderLen, byte *jagData, int *jagDataLen)
{
	// Casting to a structure makes it easier to read
	patchHeader_t *header = (patchHeader_t *)lumpData;
	jagPatchHeader_t *jagPatchHeader = (jagPatchHeader_t *)jagHeader;
	jagPatchHeader->width = swap_endian16(header->width);
	jagPatchHeader->height = swap_endian16(header->height);
	jagPatchHeader->leftoffset = swap_endian16(header->leftoffset);
	jagPatchHeader->topoffset = swap_endian16(header->topoffset);

	// Column pointers; Convert them from unsigned int to unsigned short
	for (int column = 0; column < header->width; column++)
		jagPatchHeader->columnofs[column] = swap_endian16((unsigned short)header->columnofs[column]);

	byte *dataPtr = jagData;
	unsigned short headerSize = 8 + (header->width * 2);
	byte *headerPtr = jagHeader + headerSize;

	// 'Draw' the PC Doom graphic into the Jaguar one
	for (int i = 0; i < header->width; i++)
	{
		unsigned short colOffset = (unsigned short)header->columnofs[i];
		const post_t *post = (post_t *)(lumpData + colOffset);

		jagPatchHeader->columnofs[i] = swap_endian16(colOffset);
		jagPost_t *jagPost = (jagPost_t *)headerPtr;

		while (post->topdelta != 255)
		{
			byte len = post->length;
			jagPost->topdelta = post->topdelta;
			jagPost->length = len;
			jagPost->dataofs = swap_endian16(dataPtr - jagData);
			headerSize += sizeof(jagPost_t);

			const byte *pixel = post->data;

			for (int j = 0; j < post->length; j++)
				*dataPtr++ = *pixel++;

			pixel++; // dummy value in PC gfx
			post = (const post_t *)pixel;
		}
		headerPtr += sizeof(jagPost_t);
	}

	*dataPtr++ = 0xff;

	*jagHeaderLen = headerPtr - jagHeader;
	*jagDataLen = dataPtr - jagData;
}

byte *ConvertSpriteDataFromJagToPC(byte *jagHeader, int jagHeaderLen, byte *jagData, int jagDataLen, int *outputLen)
{
	/*int key_ptr = *(int *)&table[(lump * 16)] - 0xC;
	int key_size = *(int *)&table[(lump * 16) + 4];
	int data_ptr = *(int *)&table[(lump * 16) + 16] - 0xC;
	int data_size = *(int *)&table[(lump * 16) + 20];

	byte *dataReturned = (byte *)malloc(128 * 1024); // TODO: better calculate this size. A jag sprite can't exceed this, AFAIK
	byte *drPtr = dataReturned; // You can use this cursor to move as you write data
	byte *jhPtr = jagHeader; // Jag header pointer
	byte *jdPtr = jagData; // Jag data pointer

	byte *uncompressed;
	byte *written_bytes;
	byte *line_lookup;

	int line_lookup_size;

	int array_size = 0;
	short prev_array_size = 0;

	int word;
	short location;

	int bytes_to_go;
	int bytes_copied_key;
	int bytes_copied_data;
	int byte_count;

	uncompressed = (byte *)malloc(1);
	written_bytes = (byte *)malloc(1);

	// Width
	WRITEUSHORT(drPtr, swap_endian16(READUSHORT(jhPtr))); // write to dataReturned, not fwrite()
	jhPtr += 2; // READUSHORT does *not* increment pointer!

	// Height
	WRITEUSHORT(drPtr, swap_endian16(READUSHORT(jhPtr))); // WRITEUSHORT auto-increments the pointer
	jhPtr += 2;

	// X offset
	fputc(data[key_ptr + 5], out_file);
	fputc(data[key_ptr + 4], out_file);
	// Y offset
	fputc(data[key_ptr + 7], out_file);
	fputc(data[key_ptr + 6], out_file);

	int i = (data[key_ptr + 8] << 8) | data[key_ptr + 9];
	int n = 0;

	bytes_copied_key = 0;
	bytes_to_go = key_size - i;
	while (i < key_size - 2)
	{
		word = (data[key_ptr + i] << 8) | data[key_ptr + i + 1];
		i += 2;
		if (word == 0xFFFF) {
			// MAKE IT DETECT OTHER POSSIBLE INSTANCES OF 0xFFFF FIRST!!!
			if (i < key_size - 2)
			{
				word <<= 24;

				for (n = 0; data[key_ptr + i] == 0xFF && data[key_ptr + i + 1] == 0xFF; n++)
					i += 2;
				word |= (n << 16);
				word |= ((data[key_ptr + i] << 8) | data[key_ptr + i + 1]);
				i += 2;
			}
		}

		location = ((data[key_ptr + i] << 8) | data[key_ptr + i + 1]) - 1;
		i += 2;

		if (word < 0)
		{
			prev_array_size = array_size;
			array_size = location + 3 + bytes_copied_key + ((word >> 16) & 0xFF);
			uncompressed = (byte *)realloc(uncompressed, array_size);
			written_bytes = (byte *)realloc(written_bytes, array_size);

			while (prev_array_size < array_size)
			{
				written_bytes[prev_array_size] = 0;
				prev_array_size++;
			}

			for (int n = 0; n <= (byte)((word >> 16) & 0xFF); n++)
			{
				uncompressed[location + bytes_copied_key + n] = 0xFF;
				written_bytes[location + bytes_copied_key + n] = 1;
			}

			uncompressed[location + bytes_copied_key + n] = word >> 8;
			uncompressed[location + bytes_copied_key + n + 1] = word;

			written_bytes[location + bytes_copied_key + n] = 1;
			written_bytes[location + bytes_copied_key + n + 1] = 1;

			bytes_copied_key += (3 + ((word >> 16) & 0xFF));
		}
		else
		{
			prev_array_size = array_size;
			array_size = location + 2 + bytes_copied_key;
			uncompressed = (byte *)realloc(uncompressed, array_size);
			written_bytes = (byte *)realloc(written_bytes, array_size);

			while (prev_array_size < array_size)
			{
				written_bytes[prev_array_size] = 0;
				prev_array_size++;
			}

			uncompressed[location + bytes_copied_key] = word >> 8;
			uncompressed[location + bytes_copied_key + 1] = word;

			written_bytes[location + bytes_copied_key] = 1;
			written_bytes[location + bytes_copied_key + 1] = 1;

			bytes_copied_key += 2;
		}
	}

	bytes_copied_data = 0;
	for (n = 0; n < data_size; n++)
	{
		if ((n + bytes_copied_data) >= array_size)
		{
			array_size++;

			uncompressed = (byte *)realloc(uncompressed, array_size);
			written_bytes = (byte *)realloc(written_bytes, array_size);

			written_bytes[n + bytes_copied_data] = 0;
		}
		if (written_bytes[n + bytes_copied_data])
		{
			bytes_copied_data++;
			n--;
		}
		else
			uncompressed[n + bytes_copied_data] = data[data_ptr + n];
	}

	for (int i = 0; i < array_size; i++)
	{
		if (uncompressed[i] == 0xFF)
			n++;
	}

	line_lookup_size = (n + 1) << 2;
	line_lookup = (byte *)malloc(line_lookup_size);

	n = 0;
	*(int *)&line_lookup[0] = line_lookup_size + 8;
	for (int i = 0; i < array_size; i++)
	{
		if (uncompressed[i] == 0xFF)
		{
			n++;
			*(int *)&line_lookup[n << 2] = line_lookup_size + 8 + i + 1;
		}
	}

	fwrite(line_lookup, 1, line_lookup_size, out_file);

	i = (*(int *)&line_lookup[n << 2]) + 1 - line_lookup_size - 8;
	byte_count = uncompressed[i] + 4;
	uncompressed = (byte *)realloc(uncompressed, array_size + 2);
	uncompressed[array_size] = 0;
	uncompressed[array_size + 1] = 0;

	while (uncompressed[i - 1] != 0 || uncompressed[i] != 0)
	{
		i += byte_count;
		byte_count = uncompressed[i] + 4;
	}
	array_size = i;
	free(line_lookup);

	uncompressed = (byte *)realloc(uncompressed, array_size);
	uncompressed[array_size - 1] = 0xFF;

	while (array_size & 3)
	{
		array_size++;
		uncompressed = (byte *)realloc(uncompressed, array_size);
		uncompressed[array_size - 1] = 0;
	}

	fwrite(uncompressed, 1, array_size, out_file);

	WriteTable(lump, out_file_size, array_size + line_lookup_size + 8);
	out_file_size += (array_size + line_lookup_size + 8);

	free(uncompressed);
	free(written_bytes);

	// TODO: Write to outputLen
	// *outputLen = final size of dataReturned;
	return dataReturned;*/

	return NULL;
}
