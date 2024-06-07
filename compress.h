#pragma once

#include <stddef.h>

typedef struct {
	long destOffset;
	long srcOffset;
	char copyCount;
} COMPRESSION_KEY;

enum
{
	//					32X							JAG
	THINGS,		// 0	compressed					compressed
	LINEDEFS,	// 1	compressed					compressed
	SIDEDEFS,	// 2	compressed					compressed
	VERTEXES,	// 3	big-endian, long words		compressed
	SEGS,		// 4	compressed					compressed
	SSECTORS,	// 5	compressed					compressed
	NODES,		// 6	big-endian, long words		compressed
	SECTORS,	// 7	compressed					compressed
	REJECT,		// 8	(no differences)			compressed
	BLOCKMAP	// 9	big-endian					compressed
};


// Make these public...
void *compress(void *data, size_t inputLen, size_t *outputLen);

// Make these private...
COMPRESSION_KEY *CreateKeys(void *data, int inputLen);
void *CreateKeyBits(COMPRESSION_KEY *keys, int inputLen);
void *CreateCompressedData(void *uncompressed, COMPRESSION_KEY *keys, byte *keyBits, size_t inputLen, size_t *outputLen);
