#include "Importer_Jaguar.h"
#include <stdlib.h>
#include <string.h>
#include "decompress.h"

typedef struct
{
	int32_t filepos;
	int32_t size;
	char name[8];
} directory_t;

static void DumpData(const char *filename, const byte *data, size_t length)
{
	char path[2048];
	sprintf(path, "./dump/%s", filename);

	FILE *f = fopen(path, "wb");
	fwrite(data, length, 1, f);
	fclose(f);
}

WADEntry *Importer_Jaguar::Execute()
{
	fseek(in_file, 0, SEEK_END);
	long file_size = ftell(in_file);
	rewind(in_file);

	size_t lump_count;
	intptr_t table_ptr;

	char FOURCC[5];
	FOURCC[4] = '\0';

	// Read the file signature to verify the supplied file is an IWAD or PWAD.
	fread(FOURCC, 4, 1, in_file);
	if (!strcmp(FOURCC, "IWAD") || !strcmp(FOURCC, "PWAD"))
	{
		// "DAWI" or "DAWP" found
		fread(&lump_count, 4, 1, in_file);
		lump_count = swap_endian32(lump_count);

		// limit based on "MAXLUMPS" in Jaguar source - NOPE WE ARE NOW LIMITLESS MUAHAHAHA
		fread(&table_ptr, 4, 1, in_file);
		table_ptr = swap_endian32(table_ptr);
		if (table_ptr < 0xC || table_ptr > file_size)
		{
			printf("ERROR: WAD data not found.\n");
			return NULL;
		}
	}
	else
	{
		// "IWAD" or "PWAD" not found
		printf("ERROR: WAD data not found.\n");
		return NULL;
	}

	WADEntry *wadEntries = NULL;

	// Buffer the entire file into RAM
	byte *data = (byte *)malloc(file_size);
	fseek(in_file, 0, SEEK_SET);
	fread(data, file_size, 1, in_file);

	fseek(in_file, table_ptr, SEEK_SET);

	// Grab the directory information
	// We need this because the reported sizes aren't the real sizes;
	// You need to look at the next entry for that information.
	size_t directorySize = 16 * lump_count;
	directory_t *directory = new directory_t[lump_count];
	fread(directory, 16, lump_count, in_file);

	// Fix endian-ness
	for (size_t i = 0; i < lump_count; i++)
	{
		directory_t *dirEntry = &directory[i];
		dirEntry->filepos = swap_endian32(dirEntry->filepos);
		dirEntry->size = swap_endian32(dirEntry->size);
	}

	char prevEntry[9];
	prevEntry[0] = '\0';
	strcpy(prevEntry, "unknown");

	char entryName[9];
	entryName[8] = '\0';
	for (size_t i = 0; i < lump_count; i++)
	{
		uint32_t ptr = directory[i].filepos;

		uint32_t size = directory[i].size;

		strncpy(entryName, directory[i].name, 8);

		byte *entryData = &data[ptr]; // data

		WADEntry *entry = new WADEntry();
		entry->SetIsCompressed(SetEntryName(entryName, entryName));
		entry->SetName(entryName);

		if (entry->IsCompressed())
		{
			if (i == lump_count - 1) // last entry, so we need to go by file size
			{
				uint32_t realSize = file_size;
				realSize -= ptr;

				entry->SetDataInternal(entryData, realSize);
			}
			else
			{
				directory_t *nextEntryInfo = &directory[i + 1];
				uint32_t realSize = nextEntryInfo->filepos;
				realSize -= ptr;

				entry->SetDataInternal(entryData, realSize);
			}
			entry->SetUnCompressedDataLength(size);
		}
		else
		{
			entry->SetDataInternal(entryData, size);
			strcpy(prevEntry, entry->GetName());
		}

		Listable::Add(entry, (Listable **)&wadEntries);
	}

	free(data);
	delete[] directory;

	return wadEntries;
}

Importer_Jaguar::Importer_Jaguar(FILE *f)
{
	in_file = f;
}

Importer_Jaguar::~Importer_Jaguar()
{
	fclose(in_file);
}
