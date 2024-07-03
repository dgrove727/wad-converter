#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void *memdup(const void *mem, size_t size)
{
    void *out = malloc(size);

    if (out != NULL)
        memcpy(out, mem, size);

    return out;
}

uint32_t swap_endian32(uint32_t i)
{
    return (i >> 24) | ((i >> 8) & 0x0000FF00) | ((i << 8) & 0x00FF0000) | (i << 24);
}

uint16_t swap_endian16(uint16_t i)
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

// Loads a WAD entry name and also tells you if it's marked as compressed
bool SetEntryName(char *entryName, const char *data)
{
    strncpy(entryName, data, 8);
    entryName[8] = '\0';

    bool isCompressed = (entryName[0] >> 7) != 0;
    entryName[0] = entryName[0] & 127;

    return isCompressed;
}

byte *ReadAllBytes(const char *filename, int32_t *file_size)
{
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    *file_size = (int)ftell(f);
    rewind(f);

    byte *buffer = (byte *)malloc(*file_size);
    fread(buffer, *file_size, 1, f);

    fclose(f);

    return buffer;
}

void WriteAllBytes(const char *filename, const byte *data, size_t len)
{
    FILE *f = fopen(filename, "wb");
    fwrite(data, len, 1, f);
    fclose(f);
}
