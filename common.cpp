#include "common.h"
#include <string.h>
#include <stdlib.h>

void *memdup(const void *mem, size_t size)
{
    void *out = malloc(size);

    if (out != NULL)
        memcpy(out, mem, size);

    return out;
}

unsigned int swap_endian32(unsigned int i)
{
    return (i >> 24) + ((i >> 8) & 0x0000FF00) + ((i << 8) & 0x00FF0000) + (i << 24);
}

unsigned short swap_endian16(unsigned short i)
{
    return (i >> 8) | (i << 8);
}

bool strStartsWith(const char *base, const char *str) {
    return (strstr(base, str) - base) == 0;
}

bool strEndsWith(const char *str, const char *suffix)
{
    if (str == NULL || suffix == NULL)
        return false;

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
        return false;

    return 0 == strncmp(str + str_len - suffix_len, suffix, suffix_len);
}