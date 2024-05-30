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

void ConvertSpriteDataFromPCToJag(byte *lumpData, int lumpSize, byte *jagHeader, int *jagHeaderLen, byte *jagData, int *jagDataLen)
{
	int data_ptr = *(int *)&table[(lump * 16)] - 0xC;
	int data_size = *(int *)&table[(lump * 16) + 4];

	short width;
	short height;
	short x_offset;
	short y_offset;

	int di = 0;		// data 'i'
	int ki = 0;		// key_data 'i'
	int ri = 0;		// raw_data 'i'

	int i = 0;
	int line = 0;

	int word = 0;
	int byte_count = 0;
	int bytes_copied = 0;

	byte *key_table;
	byte *key_data;
	byte *raw_data;

	// Width
	width = (data[data_ptr + 1] << 8) | data[data_ptr + 0];
	fputc(data[data_ptr + 1], out_file);
	fputc(data[data_ptr + 0], out_file);
	// Height
	height = (data[data_ptr + 3] << 8) | data[data_ptr + 2];
	fputc(data[data_ptr + 3], out_file);
	fputc(data[data_ptr + 2], out_file);
	// X offset
	x_offset = (data[data_ptr + 5] << 8) | data[data_ptr + 4];
	fputc(data[data_ptr + 5], out_file);
	fputc(data[data_ptr + 4], out_file);
	// Y offset
	y_offset = (data[data_ptr + 7] << 8) | data[data_ptr + 6];
	fputc(data[data_ptr + 7], out_file);
	fputc(data[data_ptr + 6], out_file);

	key_table = (byte *)malloc(width << 1);	// 32X table uses shorts as opposed to longs
	key_data = (byte *)malloc(1);
	raw_data = (byte *)malloc(1);

	while (line < width)
	{
		word = data[data_ptr + (width << 2) + 8 + di];

		if (word != 0xFF)
		{
			word <<= 8;
			word |= data[data_ptr + (width << 2) + 8 + di + 1];
			di += 2;
		}
		else
		{
			word = 0xFFFF;
			di++;
		}

		if (word == 0xFFFF)
		{
			line++;
			if (line < width)
			{
				for (i = 0; line < width && data[data_ptr + (width << 2) + 8 + di + i] == 0xFF; i++)
					line++;

				di += i;

				i++;
				key_data = (byte *)realloc(key_data, ki + (i << 1));
				while (i > 0)
				{
					*(short *)&key_data[ki] = 0xFFFF;
					ki += 2;
					i--;
				}
			}
			else
			{
				key_data = (byte *)realloc(key_data, ki + 2);
				*(short *)&key_data[ki] = 0xFFFF;
				ki += 2;
			}
		}
		else
		{
			key_data = (byte *)realloc(key_data, ki + 4);
			key_data[ki] = word >> 8;
			key_data[ki + 1] = word;

			*(short *)&key_data[ki + 2] = swap_endian(bytes_copied + 1) >> 16;
			ki += 4;

			raw_data = (byte *)realloc(raw_data, ri + (word & 0xFF) + 2);
			byte_count = (word & 0xFF) + 2;
			for (i = 0; i < byte_count; i++)
			{
				raw_data[ri] = data[data_ptr + (width << 2) + 8 + di];
				di++;
				ri++;
			}
			bytes_copied += i;
		}
	}

	while (ri & 3)
	{
		ri++;
		raw_data = (byte *)realloc(raw_data, ri);
		raw_data[ri - 1] = 0;
	}

	*(short *)&key_table[0] = swap_endian((width << 1) + 8) >> 16;

	i = 0;
	for (line = 1; line < width; line++)
	{
		while (*(unsigned short *)&key_data[i] != 0xFFFF)
			i += 2;

		*(short *)&key_table[line << 1] = swap_endian((width << 1) + 8 + i + 2) >> 16;
		i += 2;
	}

	fwrite(key_table, 2, width, out_file);
	fwrite(key_data, 1, ki, out_file);
	WriteTable(lump, out_file_size, (width << 1) + ki + 8);
	out_file_size += ((width << 1) + ki + 8);

	fwrite(raw_data, 1, ri, out_file);
	WriteTableCustom(out_file_size, ri, ".");
	out_file_size += ri;

	free(key_table);
	free(key_data);
	free(raw_data);
}

byte *ConvertSpriteDataFromJagToPC(byte *jagHeader, int jagHeaderLen, byte *jagData, int jagDataLen, int *outputLen)
{
	int key_ptr = *(int *)&table[(lump * 16)] - 0xC;
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
	return dataReturned;
}