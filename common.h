#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint8_t byte;
#define FRACBITS 16

// Doom GFX has a header, and then pieces of data called
// 'posts', which are offset (topdelta) from the TOP of the image (ALWAYS, even if it's a mid-column post!)
// are stored in the rest of the file, with a byte marker separating each one to indicate
// if a new row should be started or not
typedef struct
{
	uint16_t width;
	uint16_t height;
	int16_t leftoffset;
	int16_t topoffset;
	uint32_t columnofs[8];
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
	int16_t width;
	int16_t height;
	int16_t leftoffset;
	int16_t topoffset;
	uint16_t columnofs[8];
} jagPatchHeader_t;

typedef struct
{
	byte topdelta;
	byte length;
	uint16_t dataofs;
} jagPost_t;

void *memdup(const void *mem, size_t size);
uint32_t swap_endian32(uint32_t i);
uint16_t swap_endian16(uint16_t i);

bool strStartsWith(const char *base, const char *str);
bool strEndsWith(const char *str, const char *suffix);

bool SetEntryName(char *entryName, const char *data);

byte *ReadAllBytes(const char *filename, int32_t *file_size);
void WriteAllBytes(const char *filename, const byte *data, size_t len);
