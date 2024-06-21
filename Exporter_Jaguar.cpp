#include "Exporter_Jaguar.h"
#include "Texture1.h"
#include "gfx_convert.h"
#include <string.h>

const char *exclusionList[] = {
	"P1_START",
	"P1_END",
	"P2_START",
	"P2_END",
	"F1_START",
	"F1_END",
	"F2_START",
	"F2_END",
	"TEXTURE1",
	"TEXTURE2",
	"PNAMES",
	"DEMO1",
	"DEMO2",
	"DEMO3",
	"DEMO4",
	"ENDOOM",
	"GENMIDI",
	"DMXGUS",
	NULL,
};

static bool IsInExclusionList(const char *entryName)
{
	for (int i = 0; exclusionList[i] != NULL; i++)
	{
		if (!strcmp(entryName, exclusionList[i]))
			return true;
	}

	return false;
}

typedef struct
{
	int filepos;
	int size;
	char name[8];
} directory_t;

void Exporter_Jaguar::Execute()
{
	size_t lump_count = Listable::GetCount(this->entries);

	// Build our directory information first
	size_t directorySize = 16 * lump_count;
	directory_t *directory = new directory_t[lump_count];

	size_t i = 0;
	size_t fileposCursor = 0xC + directorySize; // Start of entry data
	for (WADEntry *node = entries; node; node = (WADEntry *)node->next, i++)
	{
		directory_t *dirEntry = &directory[i];

		char destName[9];
		memset(destName, 0, 9);
		strcpy(destName, node->GetName());
		memcpy(dirEntry->name, destName, 8);

		if (node->IsCompressed())
			dirEntry->name[0] |= 128; // Set the upper bit

		dirEntry->filepos = swap_endian32(fileposCursor);
		dirEntry->size = swap_endian32(node->GetUnCompressedDataLength());

		fileposCursor += node->GetDataLength(); // The real size, uncompressed or not, dictates the file pointer
	}

	fwrite("IWAD", 1, 4, f);
	unsigned int swapped_lump_count = swap_endian32(lump_count);
	fwrite(&swapped_lump_count, 4, 1, f);
	unsigned int swapped_table_ptr = swap_endian32((unsigned int)0xC);
	fwrite(&swapped_table_ptr, 4, 1, f);

	fwrite(directory, sizeof(directory_t), lump_count, f);

	// Now we just spill all of the data
	for (WADEntry *node = entries; node; node = (WADEntry *)node->next)
	{
		if (node->GetDataLength() > 0)
			fwrite(node->GetData(), node->GetDataLength(), 1, f);
	}
}

void Exporter_Jaguar::SetMaskedInTexture1()
{
	WADEntry *texture1 = WADEntry::FindEntry(entries, "TEXTURE1");

	Texture1 *t1;
	if (texture1->IsCompressed())
	{
		byte *decomp = texture1->Decompress();
		t1 = new Texture1(decomp, texture1->GetUnCompressedDataLength());
		free(decomp);
	}
	else
		t1 = new Texture1(texture1->GetData(), texture1->GetUnCompressedDataLength());

	for (MapTexture *mt = t1->mapTextures; mt; mt = (MapTexture *)mt->next)
	{
		bool inTextures = false;
		for (WADEntry *node = entries; node; node = (WADEntry *)node->next)
		{
			if (!strcmp(node->GetName(), "T_START"))
			{
				inTextures = true;
				continue;
			}

			if (!strcmp(node->GetName(), "T_END"))
			{
				inTextures = false;
				break;
			}

			if (inTextures)
			{
				char entryName[9];
				memset(entryName, 0, 9);
				memcpy(entryName, mt->name, 8);
				if (!strcmp(node->GetName(), mt->name))
				{
					// Check this texture if it contains any transparent pixels.
					// If so, we need to set masked to 1.
					if (node->IsCompressed())
					{
						byte *decomp = node->Decompress();
						if (ContainsPixel(decomp, mt->width, mt->height, 255))
							mt->masked = 1;
						free(decomp);
					}
					else
					{
						if (ContainsPixel(node->GetData(), mt->width, mt->height, 255))
							mt->masked = 1;
					}
				}
			}
		}
	}

	int newLumpLength;
	byte *newLump = t1->CreateLump(&newLumpLength);
	texture1->SetData(newLump, newLumpLength);
}

Exporter_Jaguar::Exporter_Jaguar(WADEntry *entries, FILE *f)
{
	this->entries = entries;
	this->f = f;
}

Exporter_Jaguar::~Exporter_Jaguar()
{
	// Any Cleanup?
	fclose(f);
}
