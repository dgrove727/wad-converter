#ifndef __LZLIB_H__
#define __LZLIB_H__

#include <stdint.h>

uint8_t *encode(const uint8_t *input, int32_t inputlen, int32_t *size);
void decode(uint8_t *input, uint8_t *output);
int32_t decodedsize(uint8_t *input);
void derror(char *msg);

#endif
