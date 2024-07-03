#include "Exporter_PC.h"
#include <string.h>

typedef struct
{
	int32_t filepos;
	int32_t size;
	char name[8];
} directory_t;

void Exporter_PC::Execute()
{
	size_t lump_count = Listable::GetCount(this->entries);

	// Build our directory information first
	uint32_t directorySize = (uint32_t)(16 * lump_count);
	directory_t *directory = new directory_t[lump_count];

	size_t i = 0;
	uint32_t fileposCursor = 0xC + directorySize; // Start of entry data
	for (WADEntry *node = entries; node; node = (WADEntry *)node->next, i++)
	{
		directory_t *dirEntry = &directory[i];

		char destName[9];
		memset(destName, 0, 9);
		strcpy(destName, node->GetName());
		memcpy(dirEntry->name, destName, 8);

		dirEntry->filepos = fileposCursor;
		dirEntry->size = node->GetUnCompressedDataLength();

		fileposCursor += node->GetDataLength(); // The real size, uncompressed or not, dictates the file pointer
	}

	fwrite("IWAD", 1, 4, f);
	fwrite(&lump_count, 4, 1, f);
	unsigned int table_ptr = (unsigned int)0xC;
	fwrite(&table_ptr, 4, 1, f);

	fwrite(directory, sizeof(directory_t), lump_count, f);

	// Now we just spill all of the data
	for (WADEntry *node = entries; node; node = (WADEntry *)node->next)
	{
		if (node->IsCompressed() && node->GetDataLength() > 0)
		{
			// TODO: Decompress the entry on-the-fly
			// fwrite(decompData, node->GetUnCompressedDataLength(), 1, f);
		}

		if (node->GetDataLength() > 0)
			fwrite(node->GetData(), node->GetDataLength(), 1, f);
	}
}

Exporter_PC::Exporter_PC(WADEntry *entries, FILE *f)
{
	this->entries = entries;
	this->f = f;
}

Exporter_PC::~Exporter_PC()
{
	// Any Cleanup?
	fclose(f);
}
