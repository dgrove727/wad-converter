#include "Importer_Jaguar.h"
#include <stdlib.h>
#include <string.h>
#include "decompress.h"

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

	size_t directorySize = 16 * lump_count;
	size_t dataSize = file_size - table_ptr; // 0xC is WAD header size (12 bytes)

	WADEntry *wadEntries = NULL;

	// Buffer all of the directory + entry data in RAM
	byte *data = (byte *)malloc(dataSize);
	fseek(in_file, table_ptr, SEEK_SET);
	fread(data, file_size - table_ptr, 1, in_file);

	fseek(in_file, table_ptr, SEEK_SET);

	char prevEntry[9];
	prevEntry[0] = '\0';
	strcpy(prevEntry, "unknown");

	char entryName[9];
	entryName[8] = '\0';
	for (size_t i = 0; i < lump_count; i++)
	{
		unsigned int ptr;
		fread(&ptr, 4, 1, in_file);		// ptr
		ptr = swap_endian32(ptr);

		unsigned int size;
		fread(&size, 4, 1, in_file);		// size
		size = swap_endian32(size);

		fread(entryName, 1, 8, in_file);		// name
		byte *entryData = &data[ptr - table_ptr]; // data

		WADEntry *entry = new WADEntry();
		entry->SetIsCompressed(SetEntryName(entryName, entryName));
		entry->SetName(entryName);

		if (!strcmp("TEXTURE1", entryName))
		{
			printf("hi\n");
		}

		if (entry->IsCompressed())
		{
			// Decompress it here, on the fly
			entry->SetIsCompressed(false);

			strcpy(prevEntry, entry->GetName());

			byte *decompData = decompress(entryData, size);
			entry->SetData(decompData, size);
			free(decompData);

			if (strstr(".", entry->GetName()))
			{
				char exportFile[16];
				sprintf(exportFile, "%s_c.lmp", prevEntry);
				DumpData(exportFile, (const byte *)entry->GetData(), entry->GetDataLength());
				continue;
			}


			char exportFile[16];
			sprintf(exportFile, "%s.lmp", entry->GetName());
			DumpData(exportFile, (const byte *)entry->GetData(), entry->GetDataLength());

			/*
			int getidbyte = 0;
			int len;
			int pos;
			int i;
			unsigned char* source;
			int idbyte = 0;

			while (1)
			{

				// get a new idbyte if necessary
				if (!getidbyte) idbyte = *input++;
				getidbyte = (getidbyte + 1) & 7;

				if (idbyte & 1)
				{
					// decompress
					pos = *input++ << LENSHIFT;
					pos = pos | (*input >> LENSHIFT);
					source = output - pos - 1;
					len = (*input++ & 0xf) + 1;
					if (len == 1) break;
					for (i = 0; i < len; i++)
						*output++ = *source++;
				}
				else {
					*output++ = *input++;
				}

				idbyte = idbyte >> 1;

			}

			entry->SetData(output, size);*/
		}
		else
		{
			entry->SetData(entryData, size);
			strcpy(prevEntry, entry->GetName());

			// Dump Test
			char exportFile[16];
			sprintf(exportFile, "%s.lmp", entry->GetName());
			DumpData(exportFile, (byte *)entry->GetData(), entry->GetDataLength());
		}

		Listable::Add(entry, (Listable **)&wadEntries);
	}

	free(data);

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
