#include "Importer_PC.h"
#include <stdlib.h>
#include <string.h>

WADEntry *Importer_PC::Execute()
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
		fread(&table_ptr, 4, 1, in_file);
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
	byte *data = (byte *)malloc(table_ptr - 0xC);
	fread(data, 1, table_ptr - 0xC, in_file);

	char entryName[9];
	entryName[8] = '\0';
	for (size_t i = 0; i < lump_count; i++)
	{
		uint32_t ptr;
		fread(&ptr, 4, 1, in_file);		// ptr

		uint32_t size;
		fread(&size, 4, 1, in_file);		// size

		fread(entryName, 1, 8, in_file);		// name
		byte *entryData = &data[ptr - 0xC]; // data

		WADEntry *entry = new WADEntry();
		entry->SetIsCompressed(SetEntryName(entryName, entryName));
		entry->SetName(entryName);
		entry->SetData(entryData, size);
		Listable::Add(entry, (Listable **)&wadEntries);
	}

	free(data);

	return wadEntries;
}

Importer_PC::Importer_PC(FILE *f)
{
	in_file = f;
}

Importer_PC::~Importer_PC()
{
	fclose(in_file);
}
