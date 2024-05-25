#pragma once

typedef unsigned char byte;

void *memdup(const void *mem, size_t size);
int swap_endian(unsigned int i);

bool strStartsWith(const char *base, const char *str);
bool strEndsWith(const char *str, const char *suffix);
