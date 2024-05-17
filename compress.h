#pragma once

#include <stddef.h>

void *compress(void *data, size_t inputLen, size_t *outputLen);

typedef struct {
	long destOffset
	long srcOffset;
	char copyCount;
} COMPRESSION_KEY;
