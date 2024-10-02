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

#define		VERSION			1.10
#define		WAD_FORMAT		1

#ifdef CHIBI
const char *basePath = "D:\\32xrb2\\Chibi";
#else
const char *basePath = "D:\\32xrb2";
#endif

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

static void ConvertPCSpriteEntryToJagSprite(WADEntry *entry, WADEntry **list)
{
	byte *jagHeader = (byte *)malloc(8 * 1024); // 8k
	byte *jagData = (byte *)malloc(65 * 1024); // 65k (impossible to be bigger than this)
	int jagHeaderSize, jagDataSize;

	PCSpriteToJag(entry->GetData(), entry->GetDataLength(), jagHeader, &jagHeaderSize, jagData, &jagDataSize);

	entry->SetData(jagHeader, jagHeaderSize);

	WADEntry *dotEntry = new WADEntry(".", jagData, jagDataSize);
	Listable::AddAfter(dotEntry, entry, (Listable**)&list);

	free(jagHeader);
	free(jagData);
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

		if (!strcmp(node->GetName(), "S_START"))
			insideSprites = true;
		if (!strcmp(node->GetName(), "S_END"))
			insideSprites = false;

		if (insideSprites && strcmp(node->GetName(), "S_START"))
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
			sprintf(dumpName, "D:\\32xrb2\\Levels\\E1M1\\%s.lmp", node->GetName());
			node->DumpToFile(dumpName);
			i = 10;
		}
		else if (i > 0)
		{
			char dumpName[2048];
			sprintf(dumpName, "D:\\32xrb2\\Levels\\E1M1\\%s.lmp", node->GetName());
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
	WADEntry *converted = map->CreateJaguar(map->name);

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

void InsertPCLevelFromWAD(const char *wadfile, WADEntry *entries)
{
	FILE *f = fopen(wadfile, "rb");
	Importer_PC *ipc = new Importer_PC(f);
	WADEntry *mapEntries = ipc->Execute();
	delete ipc;

	WADMap *map = new WADMap(mapEntries);

	WADEntry *jagEntries = map->CreateJaguar(map->name, true);

	WADEntry *insertPoint;
	for (insertPoint = entries; insertPoint; insertPoint = (WADEntry *)insertPoint->next)
	{
		if (!strcmp(insertPoint->GetName(), "L_START"))
			break;
	}

	WADEntry *node;
	WADEntry *next;
	for (node = jagEntries; node; node = next)
	{
		next = (WADEntry *)node->next;
		Listable::RemoveNoFree(node, (Listable **)&jagEntries);
		Listable::AddAfter(node, insertPoint, (Listable **)&entries);
		insertPoint = node;
	}

	delete map;
	delete mapEntries;
}

static void MyFunTest()
{
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

	bool insideSprites = false;
	bool insideTextures = false;
	bool insideFlats = false;
	bool insideSounds = false;
	bool inside68k = false;

	WADEntry *node;
	WADEntry *next;
	for (node = importedEntries; node; node = next)
	{
		next = (WADEntry*)node->next;

		if (!strcmp(node->GetName(), "68_START"))
		{
			inside68k = true;
			WADEntry *M68Next;
			WADEntry *lastAdded = node;
			for (WADEntry *M68Node = M68kEntries; M68Node; M68Node = M68Next)
			{
				M68Next = (WADEntry*)M68Node->next;

				// Just straight-up steal 'em, since we're working with RAM copies.
				Listable::RemoveNoFree(M68Node, (Listable **)&M68kEntries);
				Listable::AddAfter(M68Node, lastAdded, (Listable **)&importedEntries);
				lastAdded = M68Node;
			}
		}
		if (!strcmp(node->GetName(), "68_END"))
			inside68k = false;
		if (!strcmp(node->GetName(), "S_START"))
			insideSprites = true;
		if (!strcmp(node->GetName(), "S_END"))
			insideSprites = false;
		if (!strcmp(node->GetName(), "T_START"))
		{
			insideTextures = true;

			// Copy in the textures
			WADEntry *lvlTextures = (WADEntry*)WADEntry::FindEntry(lvleditorEntries, "P_START")->next;
			WADEntry *lastAdded = node;
			while (strcmp(lvlTextures->GetName(), "P_END"))
			{
				WADEntry *next = (WADEntry*)lvlTextures->next;

				// Just straight-up steal 'em, since we're working with RAM copies.
				Listable::RemoveNoFree(lvlTextures, (Listable **)&lvleditorEntries);
				Listable::AddAfter(lvlTextures, lastAdded, (Listable **)&importedEntries);

				// Convert to everyone's favorite lovable row-major Jaguar format
				int texLen;
				byte *texData = PatchToJagTexture(lvlTextures->GetData(), lvlTextures->GetDataLength(), &texLen);
				lvlTextures->SetData(texData, texLen);
				free(texData);

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

				lastAdded = lvlFlats;
				lvlFlats = next;
			}
		}
		if (!strcmp(node->GetName(), "F_END"))
			insideFlats = false;
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

		if (insideSprites && strcmp(node->GetName(), "S_START"))
		{
#ifdef CHIBI
			bool chibify = strcmp(node->GetName(), "GFZDA0") && strcmp(node->GetName(), "GFZFA0") && strcmp(node->GetName(), "GFZGA0") && strcmp(node->GetName(), "GFZQA0") && strcmp(node->GetName(), "GFZRA0") && strcmp(node->GetName(), "GFZWA0");

			if (chibify)
				ConvertPCSpriteEntryToJagSpriteChibi(node, &importedEntries);
			else
				ConvertPCSpriteEntryToJagSprite(node, &importedEntries);
#else
			ConvertPCSpriteEntryToJagSprite(node, &importedEntries);
#endif
		}
	}

	InsertPCLevelFromWAD(va("%s\\Levels\\MAP01.wad", basePath), importedEntries);
	InsertPCLevelFromWAD(va("%s\\Levels\\MAP02.wad", basePath), importedEntries);
	InsertPCLevelFromWAD(va("%s\\Levels\\MAP03.wad", basePath), importedEntries);
	InsertPCLevelFromWAD(va("%s\\Levels\\pcwad.wad", basePath), importedEntries);
	InsertPCLevelFromWAD(va("%s\\Levels\\MAP30.wad", basePath), importedEntries);

	int dummySize;
	byte *dummy = ReadAllBytes(va("%s\\22pal.txt", basePath), &dummySize);
	WADEntry *dummyEntry = new WADEntry("DUMMY", dummy, dummySize);
	Listable::Add(dummyEntry, (Listable **)&importedEntries);
	free(dummy);

	// Write it out
	FILE *expF = fopen(va("%s\\doom32x.wad", basePath), "wb");
	Exporter_Jaguar *ex = new Exporter_Jaguar(importedEntries, expF);
	// Set masked bit in TEXTURE1 lump if necessary.
	ex->SetMaskedInTexture1();
	ex->Execute();
	delete ex;

	return;
}

void ChibiMap(const char *wadfile)
{
	FILE *f = fopen(wadfile, "rb");
	Importer_PC *ipc = new Importer_PC(f);
	WADEntry *mapEntries = ipc->Execute();
	delete ipc;

	WADMap *map = new WADMap(mapEntries);
	for (int i = 0; i < map->numsectors; i++)
	{
		map->sectors[i].floorheight /= 2;
		map->sectors[i].ceilingheight /= 2;
	}

	for (int i = 0; i < map->numthings; i++)
	{
		map->things[i].x /= 2;
		map->things[i].y /= 2;
	}

	for (int i = 0; i < map->numvertexes; i++)
	{
		map->vertexes[i].x /= 2;
		map->vertexes[i].y /= 2;
	}

	WADEntry *levelEntries = map->CreatePC(mapEntries->GetName());
	f = fopen(wadfile, "wb");
	Exporter_PC *exPC = new Exporter_PC(levelEntries, f);
	exPC->Execute();
	delete exPC;
}

void ChibiMaps()
{
	ChibiMap(va("%s\\Levels\\MAP01.wad", basePath));
	ChibiMap(va("%s\\Levels\\MAP02.wad", basePath));
	ChibiMap(va("%s\\Levels\\MAP03.wad", basePath));
//	ChibiMap(va("%s\\Levels\\pcwad.wad", basePath));
	ChibiMap(va("%s\\Levels\\MAP30.wad", basePath));
}

int main(int argc, char *argv[])
{
//	ChibiMaps();
	MyFunTest();
	return 0;
	printf(
		"---------------------------------\n"
		" DOOM 32X / JAGUAR WAD CONVERTER v%01.02f\n"
		" Written by Damian Grove\n"
		" %s\n"
		"---------------------------------\n"
		"\n", VERSION, __DATE__
	);

	// TODO: Add a parameter here that sets Jaguar or 32X mode

	if (argc < 4)
	{
		printf(
			"USAGE: wad32x.exe [command] [in file] [out file]\n"
			"  example 1:  wad32x.exe -mars2pc doom32x.wad doompc.wad\n"
			"  example 2:  wad32x.exe -pc2mars doompc.wad doom32x.wad\n\n"
		);
		return 0;
	}

	if (stricmp(argv[1], "-mars2pc") == 0)
	{
		FILE *in_file = fopen(argv[2], "rb");
		if (!in_file)
		{
			printf("ERROR: WAD file not found.\n");
			return 0;
		}

		Importer_Jaguar *ij = new Importer_Jaguar(in_file);
		WADEntry *importedEntries = ij->Execute();

		// Export to PC WAD here
		/*
		FILE *out_file = fopen(argv[3], "wb");
		if (!out_file)
		{
			printf("ERROR: Unable to create output file.\n");
			fclose(in_file);
			Shutdown();
		}*/

		delete ij;
		Listable::RemoveAll((Listable **)&importedEntries);
	}
	else if (stricmp(argv[1], "-pc2mars") == 0)
	{
		FILE *in_file = fopen(argv[2], "rb");
		if(!in_file)
		{
			printf("ERROR: WAD file not found.\n");
			return 0;
		}

		Importer_PC *ipc = new Importer_PC(in_file);
		WADEntry *importedEntries = ipc->Execute();

		// Export to Jag WAD here
		/*
		FILE *out_file = fopen(argv[3], "wb");
		if (!out_file)
		{
			printf("ERROR: Unable to create output file.\n");
			fclose(in_file);
			Shutdown();
		}*/

		delete ipc;
		Listable::RemoveAll((Listable **)&importedEntries);
	}
	else
		printf("ERROR: Invalid command.\n");

	return 0;
}
