#pragma once

#include <stddef.h>

typedef unsigned char byte;
#define FRACBITS 16

// Doom GFX has a header, and then pieces of data called
// 'posts', which are offset (topdelta) from the TOP of the image (ALWAYS, even if it's a mid-column post!)
// are stored in the rest of the file, with a byte marker separating each one to indicate
// if a new row should be started or not
typedef struct
{
	unsigned short width;
	unsigned short height;
	short leftoffset;
	short topoffset;
	unsigned int columnofs[8];
} patchHeader_t;

typedef struct
{
	byte topdelta;
	byte length;
	byte unused;
	byte data[256];
} post_t;

typedef struct
{
	short width;
	short height;
	short leftoffset;
	short topoffset;
	unsigned short columnofs[8];
} jagPatchHeader_t;

typedef struct
{
	byte topdelta;
	byte length;
	unsigned short dataofs;
} jagPost_t;

void *memdup(const void *mem, size_t size);
unsigned int swap_endian32(unsigned int i);
unsigned short swap_endian16(unsigned short i);

bool strStartsWith(const char *base, const char *str);
bool strEndsWith(const char *str, const char *suffix);

bool SetEntryName(char *entryName, const char *data);

byte *ReadAllBytes(const char *filename, int *file_size);
void WriteAllBytes(const char *filename, const byte *data, size_t len);
