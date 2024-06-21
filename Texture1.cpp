#include "Texture1.h"

typedef struct
{
	char name[8];
	int masked;
	short width;
	short height;
	int columnDirectory;
	short patchCount;
} maptexturedata_t;

byte *Texture1::CreateLump(int *lumpLength)
{
	byte *data = (byte *)malloc(64 * 1024); // 64k of working space for our lump

	int *ptr32 = (int*)data;

	numTextures = Listable::GetCount(mapTextures); // Just in case you didn't set it...

	// TODO: Correctly build the offsets to allow adding/removing entries
	*ptr32++ = numTextures;
	for (int i = 0; i < numTextures; i++)
		*ptr32++ = offsets[i];

	byte *ptr8 = (byte *)ptr32;

	for (const MapTexture *node = this->mapTextures; node; node = (const MapTexture *)node->next)
	{
		maptexturedata_t *mtd = (maptexturedata_t *)ptr8;

		memcpy(mtd->name, node->name, 8);
		mtd->masked = node->masked;
		mtd->width = node->width;
		mtd->height = node->height;
		mtd->columnDirectory = node->columnDirectory;
		mtd->patchCount = node->patchCount;
		
		ptr8 += 22;

		// patchInfo
		memcpy(ptr8, node->patchInfo, 10 * node->patchCount);

		ptr8 += 10 * node->patchCount;
	}

	*lumpLength = (ptr8 - data);

	return data;
}

Texture1::Texture1(const byte *lumpData, int lumpLength)
{
	const int *ptr32 = (const int *)lumpData;
	numTextures = *ptr32++;
	offsets = (int *)malloc(sizeof(int) * numTextures);

	for (int i = 0; i < numTextures; i++)
		offsets[i] = *ptr32++;

	byte *ptr8 = (byte *)ptr32;

	for (int i = 0; i < numTextures; i++)
	{
		maptexturedata_t *mtd = (maptexturedata_t *)ptr8;

		// For some reason this is skipping every other entry...

		MapTexture *mt = new MapTexture();
		Listable::Add(mt, (Listable **)&mapTextures);
		memcpy(mt->name, mtd->name, 8);
		mt->masked = mtd->masked;
		mt->width = mtd->width;
		mt->height = mtd->height;
		mt->columnDirectory = mtd->columnDirectory;
		mt->patchCount = mtd->patchCount;

		ptr8 += 22;

		// Just copy the whole array
		mt->patchInfo = (mappatch_t *)memdup(ptr8, 10 * mt->patchCount);

		ptr8 += 10 * mt->patchCount;
	}
}

Texture1::~Texture1()
{
	Listable::RemoveAll((Listable **)&mapTextures);
	free(offsets);
}
