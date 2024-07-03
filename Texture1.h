#pragma once
#include "Listable.h"
#include "common.h"
#include <stdlib.h>
#include <memory.h>

typedef struct
{
	int16_t originX;
	int16_t originY;
	int16_t patchIndex;
	int16_t stepDir;
	int16_t colormap;
} mappatch_t;



struct MapTexture : Listable
{
	char name[8];
	int32_t masked; // 4 bytes
	int16_t width;
	int16_t height;
	int32_t columnDirectory; // ignored
	int16_t patchCount; // Always 1 in Jaguar
	mappatch_t *patchInfo; // Jag only has 1

	int32_t GetBinarySize()
	{
		int size = 22;
		size += sizeof(mappatch_t) * patchCount;

		return size;
	}

	MapTexture() {}
	virtual ~MapTexture()
	{
		if (patchInfo)
			free(patchInfo);
	}
};

struct Texture1 : Listable
{
	int32_t numTextures;
	int32_t *offsets;

	MapTexture *mapTextures;

	byte *CreateLump(int32_t *lumpLength);

	Texture1(const byte *lumpData, int32_t lumpLength);
	virtual ~Texture1();
};