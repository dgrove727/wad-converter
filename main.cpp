#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "gfx_convert.h"
#include "Importer_Jaguar.h"
#include "Exporter_Jaguar.h"
#include "Importer_PC.h"
#include "Exporter_PC.h"
#include "lzss.h"
#include "CarmackCompress.h"
#include "SRB2LevelConv.h"
#include <stdarg.h>
#include "Texture1.h"
#include "MapThing.h"
#include "SpriteColumn.h"

#define MAKE_WALL_MIPMAPS
//#define MAKE_FLAT_MIPMAPS
#define MIPLEVELS 4
#define WADPTRSTART 0x38100

#define		VERSION			1.10
#define		WAD_FORMAT		1

const char *basePath = "D:\\32xrb2";
//const char* basePath = "E:\\wad32x\\wad-converter\\bin\\Release";

char *va(const char *format, ...)
{
	va_list      argptr;
	static char  string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

static void ConvertPCSpriteEntryToJagSpriteChibi(WADEntry *entry, WADEntry **list)
{
	// First, let's shrink it
	int outputLen;
	const patchHeader_t *header = (patchHeader_t *)entry->GetData();
	byte *rawData = PatchToRaw(entry->GetData(), entry->GetUnCompressedDataLength(), &outputLen, 0);
	short newWidth = header->width / 2;
	short newHeight = header->height / 2;
	byte *shrunkRaw = (byte *)malloc((newWidth) * (newHeight));

	for (int x = 0, xd = 0; x < header->width && xd < newWidth; x += 2, xd++)
	{
		for (int y = 0, yd = 0; y < header->height && yd < newHeight; y += 2, yd++)
		{
			size_t srcPixelLocation = (y * header->width) + x;
			size_t destPixelLocation = (yd * newWidth) + xd;

			shrunkRaw[destPixelLocation] = rawData[srcPixelLocation];
		}
	}

//	int pngOutputLen;
//	byte *pngData = RawToPNG(shrunkRaw, newWidth, newHeight, &pngOutputLen);
//	WriteAllBytes("D:\\32xrb2\\chibi\\test.png", pngData, pngOutputLen);

	byte *dataToUse = shrunkRaw;
	if (newWidth == 0 || newHeight == 0)
	{
		dataToUse = rawData;
		newWidth = header->width;
		newHeight = header->height;
	}

	byte *pcData = RawToPatch(dataToUse, newWidth, newHeight, &outputLen, 0);
	patchHeader_t *pcDataHeader = (patchHeader_t *)pcData;
	pcDataHeader->leftoffset = header->leftoffset / 2;
	pcDataHeader->topoffset = header->topoffset / 2;

	byte *jagHeader = (byte *)malloc(8 * 1024); // 8k
	byte *jagData = (byte *)malloc(65 * 1024); // 65k (impossible to be bigger than this)
	int jagHeaderSize, jagDataSize;

	PCSpriteToJag(pcData, outputLen, jagHeader, &jagHeaderSize, jagData, &jagDataSize);

	entry->SetData(jagHeader, jagHeaderSize);

	WADEntry *dotEntry = new WADEntry(".", jagData, jagDataSize);
	Listable::AddAfter(dotEntry, entry, (Listable **)&list);

	free(jagHeader);
	free(jagData);
}

const char *halfSprites[] = {
	// R_PrepScenery items
	"CORL",
	"SEWE",
	"KELP",
	"FWR1",
	"FWR2",
	"FWR3",
	"FWR4",
	"FWR5",
	"FWR6",
	"BUS1",
	"BUS2",
	"JPLA",
	"STLG",
	"TFLO",
	"THZP",
	"THZT",
	"BANR",
	"ESTA",
	"FLMH",
	"CBUS",
	"CNDL",
	"CTRC",

	// R_PrepMobj items
	"BOM1",
	"BOM2",
	"BOM3",
	"DUST",
	"EFIR",
	"FL01",
	"FL02",
	"FL03",
	"FL12",
	"GFZC",
	"JETF",
	"LASR",
	"LASF",
	"MISL",

	// Shields (experimental)
	"ARMA",
	"ELEM",
	"FORC",
	"MAGN",
	"WIND",

	NULL,
};

static bool IsHalfSprite(const char *name)
{
	int i = 0;
	const char *check = halfSprites[0];
	while (check)
	{
		if (strstr(name, check) == name)
			return true;

		check = halfSprites[++i];
	}

	return false;
}

static size_t ConvertStandardGraphicToMaskedGraphic(WADEntry* entry)
{
	// Prepare buffer for reading and converting.

	byte* rawData = (byte*)entry->GetData();

	uint16_t width = swap_endian16(*(short*)&rawData[0]);
	uint16_t height = swap_endian16(*(short*)&rawData[2]);

	rawData += 16;

	int buffer_size = (320 * height) + 2;
	byte* source = (byte *)calloc(buffer_size, 1);
	for (int rowStart = 0; rowStart < height*320; rowStart += 320) {
		for (int column = 0; column < width; column++) {
			source[rowStart + column] = *rawData++;	// Expand each row to 320 pixels.
		}
	}
	source[buffer_size - 2] = 0xFF;
	source[buffer_size - 1] = 0xFF;


	// Convert graphic to fast-mask format.

	byte* dest = (byte*)malloc(65536); // 64 KB is more than enough to work with.
	byte* dest_bytes = dest;
	uint16_t* source_words = (uint16_t*)source;
	uint16_t skip_count;
	uint8_t write_count;
	bool eof = false;

	while (eof == false)
	{
		skip_count = 0;
		while (*source_words == 0x0000) {
			source_words++;
			skip_count++;
		}

		//printf("[A]    0x%02X : 0x%06X\n", skip_count, ((int)source_words - (int)source));
		if (((int)source_words - (int)source) >= buffer_size - 2) {
			skip_count = 0;
			*dest_bytes++ = (byte)skip_count;	// EOF marker;
			eof = true;
		}
		else {
			while (skip_count > 255) {
				*dest_bytes++ = 0xFF;	// Skip 255
				*dest_bytes++ = 0x00;	// Write 0
				skip_count -= 255;
			}
			*dest_bytes++ = (byte)skip_count;
			write_count = 0;
			while (*source_words != 0x0000) {
				source_words++;
				write_count++;
			}
			//printf("   [B] 0x%02X : 0x%06X\n", write_count, ((int)source_words - (int)source));
			source_words -= write_count;
			//fwrite(&write_count, 1, 1, output_file);
			//fwrite(source_words, 1, write_count * 2, output_file);
			*dest_bytes++ = write_count;
			for (int i = 0; i < write_count; i++) {
				*dest_bytes++ = (byte)source_words[i];
				*dest_bytes++ = (byte)(source_words[i]>>8);
			}
			source_words += write_count;
		}
	}

	free(source);

	size_t outputSize = dest_bytes - dest;

	entry->SetData(dest, outputSize);

	free(dest);

	return outputSize;
}

static size_t ConvertPCSpriteEntryToJagSprite(WADEntry *entry, WADEntry **list)
{
	byte *jagHeader = (byte *)malloc(8 * 1024); // 8k
	byte *jagData = (byte *)malloc(65 * 1024); // 65k (impossible to be bigger than this)
	int jagHeaderSize, jagDataSize;

	if (strcmp(entry->GetName(), "GFZGATE") && strcmp(entry->GetName(), "GFZRAIL")
		&& strcmp(entry->GetName(), "GFZFENCE") && strcmp(entry->GetName(), "NULLA0")
		&& strcmp(entry->GetName(), "CEZFNC0"))
	{
		int32_t outputLen;
		byte *cropData = CropPCPatch(entry->GetData(), entry->GetDataLength(), &outputLen, 0);
		if (cropData)
			entry->SetData(cropData, outputLen);
	}

	if (IsHalfSprite(entry->GetName()))
		PCSpriteToJagNarrow(entry->GetData(), entry->GetDataLength(), jagHeader, &jagHeaderSize, jagData, &jagDataSize);
	else
		PCSpriteToJag(entry->GetData(), entry->GetDataLength(), jagHeader, &jagHeaderSize, jagData, &jagDataSize);

	entry->SetData(jagHeader, jagHeaderSize);

	WADEntry *dotEntry = new WADEntry(".", jagData, jagDataSize);
	Listable::AddAfter(dotEntry, entry, (Listable**)&list);

	free(jagHeader);
	free(jagData);

	return jagHeaderSize + jagDataSize;
}

static void InsertLevelFromFolder(WADEntry *list, const char *levelname, const char *folder)
{
	char fullPath[2048]; // 2k ought to be enough for anybody...

	// Header
	WADEntry *entry = new WADEntry();
	entry->SetName(levelname);
	Listable::Add(entry, (Listable **)&list);

	// THINGS (compressed)
	sprintf(fullPath, "%s/THINGS.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("THINGS");
	entry->SetIsCompressed(false);
	entry->ReplaceWithFile(fullPath);

	// LINEDEFS (compressed)
	sprintf(fullPath, "%s/LINEDEFS.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("LINEDEFS");
	entry->SetIsCompressed(true);
	entry->ReplaceWithFile(fullPath);

	// SIDEDEFS (compressed)
	sprintf(fullPath, "%s/SIDEDEFS.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("SIDEDEFS");
	entry->SetIsCompressed(true);
	entry->ReplaceWithFile(fullPath);

	// VERTEXES
	sprintf(fullPath, "%s/VERTEXES.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("VERTEXES");
	entry->SetIsCompressed(false);
	entry->ReplaceWithFile(fullPath);

	// SEGS (compressed)
	sprintf(fullPath, "%s/SEGS.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("SEGS");
	entry->SetIsCompressed(true);
	entry->ReplaceWithFile(fullPath);

	// SSECTORS (compressed)
	sprintf(fullPath, "%s/SSECTORS.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("SSECTORS");
	entry->SetIsCompressed(false);
	entry->ReplaceWithFile(fullPath);

	// NODES
	sprintf(fullPath, "%s/NODES.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("NODES");
	entry->SetIsCompressed(false);
	entry->ReplaceWithFile(fullPath);

	// SECTORS (compressed)
	sprintf(fullPath, "%s/SECTORS.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("SECTORS");
	entry->SetIsCompressed(true);
	entry->ReplaceWithFile(fullPath);

	// REJECT
	sprintf(fullPath, "%s/REJECT.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("REJECT");
	entry->SetIsCompressed(false);
	entry->ReplaceWithFile(fullPath);

	// BLOCKMAP
	sprintf(fullPath, "%s/BLOCKMAP.lmp", folder);
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&list);
	entry->SetName("BLOCKMAP");
	entry->SetIsCompressed(false);
	entry->ReplaceWithFile(fullPath);
}

// Sample function on how you can crop the unused space from all of the sprites in your WAD.
static void CropSprites()
{
	FILE *f = fopen("myFile.wad", "rb");

	Importer_PC *importer = new Importer_PC(f);
	WADEntry *importedEntries = importer->Execute();
	delete importer;

	bool insideSprites = false;
	WADEntry *node;
	WADEntry *next;
	for (node = importedEntries; node; node = next)
	{
		next = (WADEntry *)node->next;

		if (!strcmp(node->GetName(), "S1_START") || !strcmp(node->GetName(), "S2_START") || !strcmp(node->GetName(), "S3_START") || !strcmp(node->GetName(), "S4_START") || !strcmp(node->GetName(), "S5_START") || !strcmp(node->GetName(), "S6_START") || !strcmp(node->GetName(), "S7_START") || !strcmp(node->GetName(), "S8_START") || !strcmp(node->GetName(), "S9_START"))
		{
			insideSprites = true;
			continue;
		}

		if (!strcmp(node->GetName(), "S1_END") || !strcmp(node->GetName(), "S2_END") || !strcmp(node->GetName(), "S3_END") || !strcmp(node->GetName(), "S4_END") || !strcmp(node->GetName(), "S5_END") || !strcmp(node->GetName(), "S6_END") || !strcmp(node->GetName(), "S7_END") || !strcmp(node->GetName(), "S8_END") || !strcmp(node->GetName(), "S9_END"))
			insideSprites = false;

		if (insideSprites)
		{
			int outputLen;
			byte *newPatch = CropPCPatch(node->GetData(), node->GetDataLength(), &outputLen, 0);

			if (newPatch) // Something was cropped
				node->SetData(newPatch, outputLen);

			free(newPatch);
		}
	}

	FILE *expF = fopen("myFile-cropped.wad", "wb");
	Exporter_PC *exporter = new Exporter_PC(importedEntries, expF);
	exporter->Execute();
	delete exporter;
}

static void DumpJagMap(const char *wadFile)
{
	FILE *f = fopen(wadFile, "rb");
	Importer_Jaguar *ij = new Importer_Jaguar(f);
	WADEntry *importedEntries = ij->Execute();
	delete ij;

	int i = 0;
	WADEntry *node;
	for (node = importedEntries; node; node = (WADEntry *)node->next)
	{
		if (!strcmp(node->GetName(), "MAP01"))
		{
			char dumpName[2048];
			sprintf(dumpName, "E:\\wad32x\\wad-converter\\bin\\Release\\Levels\\E1M1\\%s.lmp", node->GetName());
			node->DumpToFile(dumpName);
			i = 10;
		}
		else if (i > 0)
		{
			char dumpName[2048];
			sprintf(dumpName, "E:\\wad32x\\wad-converter\\bin\\Release\\Levels\\E1M1\\%s.lmp", node->GetName());
			node->DumpToFile(dumpName);
			i--;
		}
	}
}

static void ConvertMapToJaguar(const char *filename, const char *exportFilename)
{
	FILE *f = fopen(filename, "rb");
	Importer_PC *ipc = new Importer_PC(f);
	WADEntry *entries = ipc->Execute();
	delete ipc;
	fclose(f);

	WADMap *map = new WADMap(entries);
	WADEntry *converted = map->CreateJaguar(map->name, 0);

	f = fopen(exportFilename, "wb");
	Exporter_PC *epc = new Exporter_PC(converted, f);
	epc->Execute();
	fclose(f);

	delete map;
	Listable::RemoveAll((Listable **)&entries);
	Listable::RemoveAll((Listable **)&converted);
}

static void SRB2MapConv(const char *filename, const char *exportFilename)
{
	FILE *f = fopen(filename, "rb");
	Importer_PC *ipc = new Importer_PC(f);
	WADEntry *entries = ipc->Execute();
	delete ipc;
	fclose(f);

	WADMap *map = new WADMap(entries);
	WADEntry *converted = ConvertSRB2Map(map);

	f = fopen(exportFilename, "wb");
	Exporter_PC *epc = new Exporter_PC(converted, f);
	epc->Execute();
	fclose(f);

	delete map;
	Listable::RemoveAll((Listable **)&entries);
	Listable::RemoveAll((Listable **)&converted);
}

void AddSingularItemRow(MapThing *list, const mapthing_t *origin, int16_t type, int count, int16_t horizontalspacing, int16_t verticalspacing, int16_t fixedangle)
{

}

void AddEmptyEntry(WADEntry *entries)
{
	WADEntry *entry = new WADEntry("NEWPAGE", NULL, 0);
	Listable::Add(entry, (Listable**)&entries);
}

Texture1 *t1;
FlatList *fList = NULL;

void InitLevelInsertStuff(WADEntry *entries)
{
	WADEntry *texture1 = WADEntry::FindEntry(entries, "TEXTURE1");

	if (texture1->IsCompressed())
	{
		byte *decomp = texture1->Decompress();
		t1 = new Texture1(decomp, texture1->GetUnCompressedDataLength());
		free(decomp);
	}
	else
		t1 = new Texture1(texture1->GetData(), texture1->GetUnCompressedDataLength());

	fList = NULL;
	WADEntry *flatNode;
	bool inFlats = false;
	for (flatNode = entries; flatNode; flatNode = (WADEntry *)flatNode->next)
	{
		if (!strcmp(flatNode->GetName(), "F_START"))
		{
			inFlats = true;
			continue;
		}
		else if (!strcmp(flatNode->GetName(), "F_END"))
			break;

		if (inFlats)
		{
			FlatList *fi = new FlatList(flatNode->GetName(), flatNode->GetUnCompressedDataLength());
			Listable::Add(fi, (Listable **)&fList);
		}
	}
}

void CleanupLevelInsertStuff()
{
	delete fList;
	delete t1;
}

#define PADDING_SIZE(x) (((x->GetDataLength() - 1) & 3) ^ 3)
size_t InsertPCLevelFromWAD(const char* wadfile, WADEntry* entries, int loadFlags, bool skipReject)
{
	FILE *f = fopen(wadfile, "rb");
	Importer_PC *ipc = new Importer_PC(f);
	WADEntry *mapEntries = ipc->Execute();
	delete ipc;

	WADMap *map = new WADMap(mapEntries);
	/*
	// Check for special items, like vertical lines of rings, etc. and generate new mapthings in their place
	MapThing *things = NULL;
	for (int i = 0; i < map->numthings; i++)
	{
		mapthing_t *mapthing = &map->things[i];

		if (mtNode->thing.type == 600) // 5 vertical rings (yellow spring)
		{
			AddSingularItemRow(things, mapthing, 300, 5, 0, 64, 0);
		}
		else if (mtNode->thing.type == 601) // 5 vertical rings (red spring)
		{
			AddSingularItemRow(things, mapthing, 300, 5, 0, 128, 0);
		}
		else if (mtNode->thing.type == 602) // 5 diagonal rings (yellow spring)
		{
			AddSingularItemRow(things, mapthing, 300, 5, 64, 64, 0);
		}
		else if (mtNode->thing.type == 603) // 10 diagonal rings (red spring)
		{
			AddSingularItemRow(things, mapthing, 300, 10, 64, 64, 0);
		}
		else if (mtNode->thing.type == 604) // Circle of rings (8)
		{
		}
		else if (mtNode->thing.type == 605) // Circle of rings (16)
		{
		}
		else
		{
			MapThing *thing = new MapThing();
			thing->thing.angle = mapthing->angle;
			thing->thing.options = mapthing->options;
			thing->thing.type = mapthing->type;
			thing->thing.x = mapthing->x;
			thing->thing.y = mapthing->y;
			Listable::Add(thing, (Listable **)&things);
		}
	}

	// Write out the list of things
	free(map->things);
	map->numthings = 0;
	MapThing *mtNode;
	map->things = (mapthing_t *)malloc(sizeof(mapthing_t) * Listable::GetCount(things));
	for (mtNode = things; mtNode; mtNode = (MapThing *)mtNode->next)
	{
		map->things[map->numthings].angle = mtNode->thing.angle;
		map->things[map->numthings].options = mtNode->thing.options;
		map->things[map->numthings].type = mtNode->thing.type;
		map->things[map->numthings].x = mtNode->thing.x;
		map->things[map->numthings].y = mtNode->thing.y;
		map->numthings++;
	}
	*/
	WADEntry *jagEntries = map->CreateJaguar(map->name, loadFlags, true, t1, fList);

	printf("%s breakdown:\n", wadfile);
	WADEntry *node;
	size_t alignedTotalSize = 0;
	size_t totalSize = 0;
	for (node = jagEntries; node; node = (WADEntry *)node->next)
	{
		if (skipReject && !strcmp(node->GetName(), "REJECT"))
			continue;

		printf("%s: %0.2fkb\n", node->GetName(), node->GetDataLength() / 1024.0f);
		totalSize += node->GetDataLength();
		alignedTotalSize += node->GetDataLength();
		alignedTotalSize += PADDING_SIZE(node);
	}

	if (skipReject)
	{
		for (node = jagEntries; node; node = (WADEntry*)node->next)
		{
			if (!strcmp(node->GetName(), "REJECT"))
			{
				printf("Removing reject..\n");
				node->SetData(NULL, 0);
				break;
			}
		}
	}

	printf("Total size: %0.2fkb\n", totalSize / 1024.0f);

	// Don't need to look for L_START, just put it at the end of the WAD
	Listable *itemNext = jagEntries->next;
	Listable::Add(jagEntries, (Listable **)&entries);
	jagEntries->next = itemNext; // Restore next link

	delete map;
	delete mapEntries;

	return alignedTotalSize;
}

static void FindDuplicateColumns(WADEntry *entries)
{
	size_t savedBytes = 0;

	SpriteColumn *columns = NULL;

	bool insideSprites = false;
	WADEntry *node;
	for (node = entries; node; node = (WADEntry *)node->next)
	{
		if (!strcmp(node->GetName(), "S1_START") || !strcmp(node->GetName(), "S2_START") || !strcmp(node->GetName(), "S3_START") || !strcmp(node->GetName(), "S4_START") || !strcmp(node->GetName(), "S5_START") || !strcmp(node->GetName(), "S6_START") || !strcmp(node->GetName(), "S7_START") || !strcmp(node->GetName(), "S8_START") || !strcmp(node->GetName(), "S9_START"))
		{
			insideSprites = true;
			continue;
		}
		if (!strcmp(node->GetName(), "S1_END") || !strcmp(node->GetName(), "S2_END") || !strcmp(node->GetName(), "S3_END") || !strcmp(node->GetName(), "S4_END") || !strcmp(node->GetName(), "S5_END") || !strcmp(node->GetName(), "S6_END") || !strcmp(node->GetName(), "S7_END") || !strcmp(node->GetName(), "S8_END") || !strcmp(node->GetName(), "S9_END"))
			break;

		if (!insideSprites)
			continue;

		if (strcmp(node->GetName(), "."))
		{
			WADEntry *postData = (WADEntry*)node->next;

			jagPatchHeader_t *header = (jagPatchHeader_t *)node->GetData();
			uint16_t width = swap_endian16(header->width);
			uint16_t height = swap_endian16(header->height);
			uint16_t leftOffset = swap_endian16(header->leftoffset);
			uint16_t topOffset = swap_endian16(header->topoffset);

			for (int i = 0; i < width; i++)
			{
				uint16_t colOffset = swap_endian16(header->columnofs[i]);
				const jagPost_t *post = (jagPost_t *)(node->GetData() + colOffset);
				uint16_t dataOffset = swap_endian16(post->dataofs);
				const byte *pixel = &postData->GetData()[dataOffset];

				if (post->topdelta == 255 && post->length == 255)
					continue;

				int z = 0;
				for (int i = 0; i < post->length; i++) // Verify the pixel data
				{
					z += pixel[i];
				}

				SpriteColumn *sCol = new SpriteColumn(pixel, post->length);
				Listable::Add(sCol, (Listable **)&columns);
			}

			node = postData;
		}
	}

	printf("There are %d sprite columns.\n", Listable::GetCount(columns));

	SpriteColumn *colNode;
	int nodeIndex = 0;
	for (colNode = columns; colNode; colNode = (SpriteColumn *)colNode->next, nodeIndex++)
	{
		if (colNode->duplicate)
			continue;

		SpriteColumn *checkNode;
		int checkIndex = 0;
		for (checkNode = (SpriteColumn*)colNode->next; checkNode; checkNode = (SpriteColumn *)checkNode->next, checkIndex++)
		{
			if (checkNode->duplicate)
				continue;

			if (colNode->Equals(checkNode))
			{
				savedBytes += checkNode->length;
				checkNode->duplicate = true;
			}
		}
	}

	printf("If sprite columns were compressed, it would save %d bytes.\n", savedBytes);
}

static void WADMapEdits()
{
	FILE* f = fopen(va("D:\\32xrb2\\Levels\\MAP11.wad", basePath), "rb");

	Importer_PC* ipc = new Importer_PC(f);
	WADEntry* importedEntries = ipc->Execute();
	delete ipc;

	WADMap* map = new WADMap(importedEntries);

	for (int i = 0; i < map->numlinedefs; i++)
	{
		linedef_t *line = &map->linedefs[i];

		if (line->special == 0 && line->sidenum[1] != -1)
		{
			sidedef_t *side1 = &map->sidedefs[line->sidenum[0]];
			sidedef_t *side2 = &map->sidedefs[line->sidenum[1]];

			if (side1->midtexture[0] == '-' && side2->midtexture[0] == '-')
			{
				side1->rowoffset = 0;
				side1->textureoffset = 0;
				side2->rowoffset = 0;
				side2->textureoffset = 0;
			}
		}
	}

	fclose(f);

	f = fopen(va("D:\\32xrb2\\Levels\\MAP11.wad", basePath), "wb");

	Exporter_PC* epc = new Exporter_PC(importedEntries, f);
	epc->Execute();
	delete epc;

	fclose(f);

	delete map;
}

size_t CalculatePageSize(WADEntry *start, WADEntry *end)
{
	size_t total = 0;

	WADEntry *node = start;
	do
	{
		total += node->GetDataLength();

		if (node == end)
			break;

		node = (WADEntry *)node->next;
	} while (true);

	return total;
}

// PNG Magic Signature: 89 50 4E 47 0D 0A 1A 0A
static const uint8_t PNG_SIGNATURE[8] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
};

bool is_png_header(const uint8_t *buffer, size_t size) {
	// Ensure buffer has at least 8 bytes
	if (size < 8) {
		return false;
	}

	// Compare the first 8 bytes directly
	return memcmp(buffer, PNG_SIGNATURE, 8) == 0;
}

int get_exponent_portable(unsigned int n)
{
	if (n == 0)
		return -1;

	int exponent = 0;
	while (n > 1)
	{
		n >>= 1; // Shift right by 1 (divide by 2)
		exponent++;
	}
	return exponent;
}

static void MyFunTest()
{
//		WADMapEdits();
//		return;

	FILE *f = fopen(va("%s\\srb32x-edit.wad", basePath), "rb");

	Importer_PC *ipc = new Importer_PC(f);
	WADEntry *importedEntries = ipc->Execute();
	delete ipc;

	f = fopen(va("%s\\leveleditor.wad", basePath), "rb"); // Where to get textures/flats/TEXTURE1
	ipc = new Importer_PC(f);
	WADEntry *lvleditorEntries = ipc->Execute();
	delete ipc;

	f = fopen(va("%s\\68k.wad", basePath), "rb"); // Where to get 68k entries (first bank)
	ipc = new Importer_PC(f);
	WADEntry *M68kEntries = ipc->Execute();
	delete ipc;

	size_t sizeSprites = 0;
	size_t sizeTextures = 0;
	size_t sizeFlats = 0;
	size_t sizeGraphics = 0;
	size_t sizeCompGraphics = 0;
	size_t sizeMaskedGraphics = 0;
	size_t sizeCompMaskedGraphics = 0;
	flatsize_t flatSizes[256]; // Only a maximum of 256 flats allowed
	size_t numFlats = 0;

	bool insideSprites = false;
	bool insideTextures = false;
	bool insideFlats = false;
	bool insideSounds = false;
	bool inside68k = false;
	bool insideCompressedMaskedGraphics = false;
	bool insideRegularMaskedGraphics = false;
	bool insideCompressedGraphics = false;
	bool insideRegularGraphics = false;

	WADEntry *node;
	WADEntry *next;
	for (node = importedEntries; node; node = next)
	{
		printf("%s\n", node->GetName());
		next = (WADEntry *)node->next;

		if (!strcmp(node->GetName(), "F_SKY1"))
			continue;

		if (!strcmp(node->GetName(), "MC_END"))
			insideCompressedMaskedGraphics = false;

		if (!strcmp(node->GetName(), "M_END"))
			insideRegularMaskedGraphics = false;

		if (!strcmp(node->GetName(), "M_START"))
			insideRegularMaskedGraphics = true;

		if (!strcmp(node->GetName(), "GC_END"))
			insideCompressedGraphics = false;

		if (!strcmp(node->GetName(), "G_END"))
			insideRegularGraphics = false;

		if (!strcmp(node->GetName(), "G_START"))
			insideRegularGraphics = true;

		if (!insideFlats && is_png_header(node->GetData(), node->GetDataLength()))
		{
			pngresult_t result = PNGTo15Bit(node->GetData(), node->GetDataLength());
			node->SetData(result.data, result.dataSize);
			printf("Converting %s to 15bpp graphic.\n", node->GetName());
		}

		if (insideRegularGraphics || insideCompressedGraphics)
		{
			// Scan for the MEGADRIVE_THRU_COLOR and replace it 
			const byte MEGADRIVE_THRU_COLOR = 0xff;

			byte *newData = (byte *)memdup(node->GetData(), node->GetDataLength());

			if (
				strcmp(node->GetName(), "CHEVBLK")// &&
				//strcmp(node->GetName(), "KFIST1") &&
				//strcmp(node->GetName(), "KFIST2") &&
				//strcmp(node->GetName(), "KFIST3") &&
				//strcmp(node->GetName(), "KFIST4") &&
				//strcmp(node->GetName(), "KFIST5") &&
				//strcmp(node->GetName(), "KFIST6") &&
				//strcmp(node->GetName(), "KFIST7") &&
				//strcmp(node->GetName(), "M_TITLEA") &&
				//strcmp(node->GetName(), "M_TITLEB") &&
				//strcmp(node->GetName(), "TAILWAG1") &&
				//strcmp(node->GetName(), "TAILWAG2") &&
				//strcmp(node->GetName(), "TAILWAG3") &&
				//strcmp(node->GetName(), "TAILWAG4") &&
				//strcmp(node->GetName(), "TAILWAG5") &&
				//strcmp(node->GetName(), "TAILWAG6")
				)
			{
				for (size_t i = 16; i < node->GetDataLength(); i++)
				{
					if (newData[i] == MEGADRIVE_THRU_COLOR)
					{
						newData[i] = 0xd0;
						printf("Found MD_THRU_COLOR in %s\n", node->GetName());
					}
				}
			}

			node->SetData(newData, node->GetDataLength());
			free(newData);
		}

		if (insideCompressedMaskedGraphics)
		{
			node->SetIsCompressed(true);
			ConvertStandardGraphicToMaskedGraphic(node);
			sizeCompMaskedGraphics += node->GetDataLength();
		}

		if (insideCompressedGraphics)
		{
			byte *data = (byte *)memdup(node->GetData(), node->GetDataLength());
			node->SetIsCompressed(true);
			node->SetData(data, node->GetDataLength());
			free(data);
			sizeCompGraphics += node->GetDataLength();
		}

		if (!strcmp(node->GetName(), "68_START"))
		{
			inside68k = true;
			WADEntry *M68Next;
			WADEntry *lastAdded = node;
			for (WADEntry *M68Node = M68kEntries; M68Node; M68Node = M68Next)
			{
				M68Next = (WADEntry *)M68Node->next;

				// Just straight-up steal 'em, since we're working with RAM copies.
				Listable::RemoveNoFree(M68Node, (Listable **)&M68kEntries);
				Listable::AddAfter(M68Node, lastAdded, (Listable **)&importedEntries);
				lastAdded = M68Node;
			}
		}
		if (!strcmp(node->GetName(), "68_END"))
			inside68k = false;
		if (!strcmp(node->GetName(), "S1_START") || !strcmp(node->GetName(), "S2_START") || !strcmp(node->GetName(), "S3_START") || !strcmp(node->GetName(), "S4_START") || !strcmp(node->GetName(), "S5_START") || !strcmp(node->GetName(), "S6_START") || !strcmp(node->GetName(), "S7_START") || !strcmp(node->GetName(), "S8_START") || !strcmp(node->GetName(), "S9_START"))
			insideSprites = true;
		if (!strcmp(node->GetName(), "S1_END") || !strcmp(node->GetName(), "S2_END") || !strcmp(node->GetName(), "S3_END") || !strcmp(node->GetName(), "S4_END") || !strcmp(node->GetName(), "S5_END") || !strcmp(node->GetName(), "S6_END") || !strcmp(node->GetName(), "S7_END") || !strcmp(node->GetName(), "S8_END") || !strcmp(node->GetName(), "S9_END"))
			insideSprites = false;
		if (!strcmp(node->GetName(), "MC_START"))
			insideCompressedMaskedGraphics = true;
		if (!strcmp(node->GetName(), "GC_START"))
			insideCompressedGraphics = true;
		if (!strcmp(node->GetName(), "T_START"))
		{
			insideTextures = true;

			// Copy in the textures
			WADEntry *lvlTextures = (WADEntry *)WADEntry::FindEntry(lvleditorEntries, "P_START")->next;
			WADEntry *lastAdded = node;
			while (strcmp(lvlTextures->GetName(), "P_END"))
			{
				WADEntry *next = (WADEntry *)lvlTextures->next;

				// Just straight-up steal 'em, since we're working with RAM copies.
				Listable::RemoveNoFree(lvlTextures, (Listable **)&lvleditorEntries);
				Listable::AddAfter(lvlTextures, lastAdded, (Listable **)&importedEntries);

				// Convert to everyone's favorite lovable col-major Jaguar format
				int texLen;
				byte *texData = PatchToJagTexture(lvlTextures->GetData(), lvlTextures->GetDataLength(), &texLen);

#ifdef MAKE_WALL_MIPMAPS
				const patchHeader_t *header = (patchHeader_t *)lvlTextures->GetData(); // Need width/height info

				int dataLen;
				byte *mipData = PatchMipmaps(texData, header->height, header->width, MIPLEVELS, &dataLen);
				free(texData);

				lvlTextures->SetData(mipData, dataLen);
				free(mipData);
#else
				//				if (!strcmp(lvlTextures->GetName(), "GFZROCK"))
				//					lvlTextures->SetIsCompressed(true);

				lvlTextures->SetData(texData, texLen);
				free(texData);
#endif
				sizeTextures += lvlTextures->GetDataLength();

				lastAdded = lvlTextures;
				lvlTextures = next;
			}
		}
		if (!strcmp(node->GetName(), "T_END"))
			insideTextures = false;
		if (!strcmp(node->GetName(), "F_START"))
		{
			insideFlats = true;

			// Copy in the flats
			WADEntry *lvlFlats = (WADEntry *)WADEntry::FindEntry(lvleditorEntries, "F_START")->next;
			WADEntry *lastAdded = node;
			while (strcmp(lvlFlats->GetName(), "F_END"))
			{
				WADEntry *next = (WADEntry *)lvlFlats->next;

				// Just straight-up steal 'em, since we're working with RAM copies.
				Listable::RemoveNoFree(lvlFlats, (Listable **)&lvleditorEntries);
				Listable::AddAfter(lvlFlats, lastAdded, (Listable **)&importedEntries);

				/*				const byte* flatData = lvlFlats->GetData();
								lvlFlats->SetIsCompressed(true);
								lvlFlats->SetData(flatData, lvlFlats->GetDataLength());*/

				// Convert from PNG to row-major raw
				int32_t flatWidth, flatHeight;
				byte *flatData = PNGToFlat(lvlFlats->GetData(), lvlFlats->GetDataLength(), &flatWidth, &flatHeight);
				lvlFlats->SetData(flatData, flatWidth * flatHeight);
				free(flatData);

				flatSizes[numFlats].width = (uint8_t)get_exponent_portable(flatWidth);
				flatSizes[numFlats].height = (uint8_t)get_exponent_portable(flatHeight);
				numFlats++;

#ifdef MAKE_FLAT_MIPMAPS
				int dataLen;
				byte *mipData = FlatMipmaps(lvlFlats->GetData(), lvlFlats->GetUnCompressedDataLength(), MIPLEVELS, &dataLen);
				lvlFlats->SetData(mipData, dataLen);
				free(mipData);
#endif
				sizeFlats += lvlFlats->GetDataLength();

				lastAdded = lvlFlats;
				lvlFlats = next;
			}
		}
		if (!strcmp(node->GetName(), "F_END"))
		{
			insideFlats = false;
			WADEntry *flatInfo = new WADEntry("FLATINFO", (const byte *)flatSizes, numFlats * 2);
			Listable::AddAfter(flatInfo, node, (Listable **)&importedEntries);
		}
		if (!strcmp(node->GetName(), "DS_START"))
		{
			insideSounds = true;
		}
		if (!strcmp(node->GetName(), "DS_END"))
		{
			insideSounds = false;
		}

		if (!strcmp(node->GetName(), "TEXTURE1"))
		{
			WADEntry *texture1lump = WADEntry::FindEntry(lvleditorEntries, "TEXTURE1");
			node->SetData(texture1lump->GetData(), texture1lump->GetDataLength());
		}

		// Rename entries that SLADE doesn't support
		if (!strcmp(node->GetName(), "PLAYz1"))
		{
			node->SetName("PLAY^1");
		}
		else if (!strcmp(node->GetName(), "PLAYz2z8"))
		{
			node->SetName("PLAY^2^8");
		}
		else if (!strcmp(node->GetName(), "PLAYz3z7"))
		{
			node->SetName("PLAY^3^7");
		}
		else if (!strcmp(node->GetName(), "PLAYz4z6"))
		{
			node->SetName("PLAY^4^6");
		}
		else if (!strcmp(node->GetName(), "PLAYz5"))
		{
			node->SetName("PLAY^5");
		}
		else if (!strcmp(node->GetName(), "IVSPz0"))
		{
			node->SetName("IVSP^0");
		}

		if (insideSprites && !strstr(node->GetName(), "_START"))
		{
#ifdef CHIBI
			bool chibify = strcmp(node->GetName(), "GFZDOOR") && strcmp(node->GetName(), "GFZFENCE") && strcmp(node->GetName(), "GFZGRASS") && strcmp(node->GetName(), "GFZGATE") && strcmp(node->GetName(), "GFZRAIL") && strcmp(node->GetName(), "GFZWINDP");

			if (chibify)
				sizeSprites += ConvertPCSpriteEntryToJagSpriteChibi(node, &importedEntries);
			else
				sizeSprites += ConvertPCSpriteEntryToJagSprite(node, &importedEntries);
#else
			sizeSprites += ConvertPCSpriteEntryToJagSprite(node, &importedEntries);
#endif
		}

		if (insideRegularMaskedGraphics && node->GetDataLength() > 0) {
			ConvertStandardGraphicToMaskedGraphic(node);
			sizeMaskedGraphics += node->GetDataLength();
		}
		if (insideRegularGraphics) {
			sizeGraphics += node->GetDataLength();
		}
	}
	
	size_t extraSpace = 0;
	WADEntry *startMarker;
	InitLevelInsertStuff(importedEntries);
	printf("---------------------Page 8:\n");
	startMarker = (WADEntry*)Listable::GetLast(importedEntries);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP01a.wad", basePath), importedEntries, 255, false);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP02a.wad", basePath), importedEntries, 47, false);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP03a.wad", basePath), importedEntries, 255, true);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP04b.wad", basePath), importedEntries, 0, false);
	printf("*********************Space used: %0.2fkb\n", CalculatePageSize(startMarker, (WADEntry*)Listable::GetLast(startMarker)) / 1024.0f);
	AddEmptyEntry(importedEntries);
	extraSpace = 0;
	printf("---------------------Page 9:\n");
	startMarker = (WADEntry *)Listable::GetLast(importedEntries);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP05a.wad", basePath), importedEntries, 0, false);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP06a.wad", basePath), importedEntries, 255, true);
	printf("*********************Space used: %0.2fkb\n", CalculatePageSize(startMarker, (WADEntry *)Listable::GetLast(startMarker)) / 1024.0f);
	AddEmptyEntry(importedEntries);
	extraSpace = 0;
	printf("---------------------Page 10:\n");
	startMarker = (WADEntry *)Listable::GetLast(importedEntries);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP07b.wad", basePath), importedEntries, 0, false);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP10a.wad", basePath), importedEntries, 0, false);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP12a.wad", basePath), importedEntries, 255, true);
	printf("*********************Space used: %0.2fkb\n", CalculatePageSize(startMarker, (WADEntry *)Listable::GetLast(startMarker)) / 1024.0f);
	AddEmptyEntry(importedEntries);
	extraSpace = 0;
	printf("---------------------Page 11:\n");
	startMarker = (WADEntry *)Listable::GetLast(importedEntries);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP11a.wad", basePath), importedEntries, 256, true);
	printf("*********************Space used: %0.2fkb\n", CalculatePageSize(startMarker, (WADEntry *)Listable::GetLast(startMarker)) / 1024.0f);
	AddEmptyEntry(importedEntries);
	extraSpace = 0;
	printf("---------------------Page 12:\n");
	startMarker = (WADEntry *)Listable::GetLast(importedEntries);
	//	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP16a.wad", basePath), importedEntries);
//	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP17.wad", basePath), importedEntries);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP30a.wad", basePath), importedEntries, 255, true);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP60a.wad", basePath), importedEntries, 255, true);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP61b.wad", basePath), importedEntries, 255, true);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP62b.wad", basePath), importedEntries, 255, true);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP63b.wad", basePath), importedEntries, 255, true);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP64b.wad", basePath), importedEntries, 255, true);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP65b.wad", basePath), importedEntries, 255, true);
	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP22a.wad", basePath), importedEntries, 0, true);
	//	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\FOF.wad", basePath), importedEntries, 255);
	//	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP65.wad", basePath), importedEntries);
	//	extraSpace += InsertPCLevelFromWAD(va("%s\\Levels\\MAP66.wad", basePath), importedEntries);
	printf("*********************Space used: %0.2fkb\n", CalculatePageSize(startMarker, (WADEntry *)Listable::GetLast(startMarker)) / 1024.0f);
	CleanupLevelInsertStuff();

	int dummySize = 4;
	byte dummy[] = { 0xaf, 0xaf, 0xaf, 0xaf };
	WADEntry *dummyEntry = new WADEntry("DUMMY", dummy, dummySize);
	Listable::Add(dummyEntry, (Listable **)&importedEntries);

//	FindDuplicateColumns(importedEntries);

	// Write it out
	FILE *expF = fopen(va("%s\\doom32x.wad", basePath), "wb");
	Exporter_Jaguar *ex = new Exporter_Jaguar(importedEntries, expF, WADPTRSTART);
	// Set masked bit in TEXTURE1 lump if necessary.
	ex->SetMaskedInTexture1();
	ex->Execute();
	delete ex;

	printf("Total sizes:\n");
	printf("Sprites: %0.2fkb\n", sizeSprites / 1024.0f);
	printf("Flats: %0.2fkb\n", sizeFlats / 1024.0f);
	printf("Textures: %0.2fkb\n", sizeTextures / 1024.0f);
	printf("Graphics: %0.2fkb\n", sizeGraphics / 1024.0f);
	printf("Compressed Graphics: %0.2fkb\n", sizeCompGraphics / 1024.0f);
	printf("Masked Graphics: %0.2fkb\n", sizeMaskedGraphics / 1024.0f);
	printf("Compressed Masked Graphics: %0.2fkb\n", sizeCompMaskedGraphics / 1024.0f);

	expF = fopen(va("%s\\doom32x.wad", basePath), "rb");
	fseek(expF, 0, SEEK_END);
	printf("Total file size: %02fkb\n", ftell(expF) / 1024.0f);
	fclose(expF);

	return;
}

int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		// PNG -> flat conversion mode
		char newfileName[2048];
		strcpy(newfileName, argv[1]);
		int len = strlen(newfileName);
		newfileName[len - 1] = 'p';
		newfileName[len - 2] = 'm';
		newfileName[len - 3] = 'l';

		int32_t pngSize;
		byte *png = ReadAllBytes(argv[1], &pngSize);

		int32_t flatWidth, flatHeight;
		byte *flat = PNGToFlat(png, pngSize, &flatWidth, &flatHeight);

		WriteAllBytes(newfileName, flat, flatWidth * flatHeight);
		free(flat);
		free(png);

		printf("Converted %s to flat graphic.\n", argv[1]);
		return 0;
	}
//	ChibiMaps();
	MyFunTest();

	return 0;
}
