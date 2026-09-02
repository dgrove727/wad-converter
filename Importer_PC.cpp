#include "Importer_PC.h"
#include <stdlib.h>
#include <string.h>

WADEntry *Importer_PC::Execute()
{
	fseek(in_file, 0, SEEK_END);
	long file_size_long = ftell(in_file);
	rewind(in_file);

	if (file_size_long < 0xC)
	{
		printf("ERROR: Not a WAD file.\n");
		return NULL;
	}

	const size_t file_size = (size_t)file_size_long;

	uint32_t lump_count = 0;
	uint32_t table_ptr = 0;

	char FOURCC[5];
	FOURCC[4] = '\0';

	// Read the file signature to verify the supplied file is an IWAD or PWAD.
	if (fread(FOURCC, 4, 1, in_file) != 1)
	{
		printf("ERROR: Failed to read WAD signature.\n");
		return NULL;
	}

	if (strcmp(FOURCC, "IWAD") && strcmp(FOURCC, "PWAD"))
	{
		printf("ERROR: IWAD/PWAD header not found.\n");
		return NULL;
	}

	if (fread(&lump_count, 4, 1, in_file) != 1 ||
		fread(&table_ptr, 4, 1, in_file) != 1)
	{
		printf("ERROR: Failed to read WAD header.\n");
		return NULL;
	}

	if (table_ptr < 0xC || (size_t)table_ptr > file_size)
	{
		printf("ERROR: Invalid WAD directory offset (0x%X).\n", table_ptr);
		return NULL;
	}

	const size_t dir_bytes = (size_t)lump_count * 16u;
	if ((size_t)table_ptr + dir_bytes > file_size)
	{
		printf("ERROR: WAD directory extends past end of file (count=%u, offset=0x%X).\n",
			lump_count, table_ptr);
		return NULL;
	}

	const size_t data_size = (size_t)table_ptr - 0xCu;
	uint8_t *data = (uint8_t *)malloc(data_size);
	if (!data)
	{
		printf("ERROR: Failed to allocate %zu bytes for WAD lump data.\n", data_size);
		return NULL;
	}

	if (fread(data, 1, data_size, in_file) != data_size)
	{
		printf("ERROR: Failed to read WAD lump data.\n");
		free(data);
		return NULL;
	}

	// Directory follows lump data at table_ptr; seek there explicitly.
	if (fseek(in_file, (long)table_ptr, SEEK_SET) != 0)
	{
		printf("ERROR: Failed to seek to WAD directory.\n");
		free(data);
		return NULL;
	}

	WADEntry *wadEntries = NULL;
	char entryName[9];
	entryName[8] = '\0';

	for (uint32_t i = 0; i < lump_count; i++)
	{
		uint32_t ptr = 0;
		uint32_t size = 0;

		if (fread(&ptr, 4, 1, in_file) != 1 ||
			fread(&size, 4, 1, in_file) != 1 ||
			fread(entryName, 1, 8, in_file) != 8)
		{
			printf("ERROR: Failed to read directory entry %u.\n", i);
			free(data);
			Listable::RemoveAll((Listable **)&wadEntries);
			return NULL;
		}

		// Marker / empty lumps are allowed (size 0)
		if (size == 0)
		{
			WADEntry *entry = new WADEntry();
			entry->SetIsCompressed(false);
			entry->SetName(entryName);
			entry->SetData(NULL, 0);
			Listable::Add(entry, (Listable **)&wadEntries);
			continue;
		}

		// Lump payload must lie entirely within the pre-directory data region.
		// Lump data starts at file offset `ptr`; our buffer holds [0xC, table_ptr).
		if (ptr < 0xCu ||
			(size_t)ptr >(size_t)table_ptr ||
			(size_t)size > data_size ||
			(size_t)(ptr - 0xCu) > data_size - (size_t)size)
		{
			printf("ERROR: Lump %u ('%.8s') has invalid ptr/size (ptr=0x%X, size=%u, data_size=%zu).\n",
				i, entryName, ptr, size, data_size);
			free(data);
			Listable::RemoveAll((Listable **)&wadEntries);
			return NULL;
		}

		uint8_t *entryData = &data[ptr - 0xCu];

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
