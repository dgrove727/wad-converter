#pragma once
#include "Listable.h"
#include "common.h"
#include <stdlib.h>
#include <memory.h>

typedef struct
{
	short originX;
	short originY;
	short patchIndex;
	short stepDir;
	short colormap;
} mappatch_t;



struct MapTexture : Listable
{
	char name[8];
	int masked; // 4 bytes
	short width;
	short height;
	int columnDirectory; // ignored
	short patchCount; // Always 1 in Jaguar
	mappatch_t *patchInfo; // Jag only has 1

	int GetBinarySize()
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
	int numTextures;
	int *offsets;

	MapTexture *mapTextures;

	byte *CreateLump(int *lumpLength);

	Texture1(const byte *lumpData, int lumpLength);
	virtual ~Texture1();
};