#pragma once

#include <stddef.h>

typedef struct {
	long destOffset;
	long srcOffset;
	char copyCount;
} COMPRESSION_KEY;


// Make these public...
void *compress(void *data, size_t inputLen, size_t *outputLen);

// Make these private...
COMPRESSION_KEY *CreateKeys(void *data, size_t inputLen);
void *CreateKeyBits(COMPRESSION_KEY *keys, size_t inputLen);
void *CreateCompressedData(COMPRESSION_KEY *keys, byte *keyBits, size_t inputLen, size_t *outputLen);
