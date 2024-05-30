#pragma once

#include <stddef.h>

typedef unsigned char byte;

void *memdup(const void *mem, size_t size);
unsigned int swap_endian32(unsigned int i);
unsigned short swap_endian16(unsigned short i);

bool strStartsWith(const char *base, const char *str);
bool strEndsWith(const char *str, const char *suffix);
