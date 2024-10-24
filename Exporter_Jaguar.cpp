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
	for (int32_t i = 0; exclusionList[i] != NULL; i++)
	{
		if (!strcmp(entryName, exclusionList[i]))
			return true;
	}

	return false;
}

typedef struct
{
	int32_t filepos;
	int32_t size;
	char name[8];
} directory_t;

void Exporter_Jaguar::Execute()
{
	size_t lump_count = Listable::GetCount(this->entries);

	// Build our directory information first
	uint32_t directorySize = (uint32_t)(16 * lump_count);
	directory_t *directory = new directory_t[lump_count];

	bool bankSwitching = wadPtrStart != 0;
	size_t i = 0;
	uint32_t fileposCursor = 0xC + directorySize; // Start of entry data
	for (WADEntry *node = entries; node; node = (WADEntry *)node->next, i++)
	{
		int32_t padding_size = ((node->GetDataLength() - 1) & 3) ^ 3;

		directory_t *dirEntry = &directory[i];

		char destName[9];
		memset(destName, 0, 9);
		strcpy(destName, node->GetName());
		memcpy(dirEntry->name, destName, 8);

		if (node->IsCompressed())
			dirEntry->name[0] |= 128; // Set the upper bit

		node->dir_entry_filepos = &dirEntry->filepos;
		dirEntry->filepos = 0;
		dirEntry->size = swap_endian32(node->GetUnCompressedDataLength());

		// Add the real size, uncompressed or not, to the file pointer.
		fileposCursor += ((node->GetDataLength() - 1) & 0xFFFFFFFC) + 4; // Maintain 4-byte alignment.
	}

#define HALFMEG (512*1024)

	// Pre-scan the entries and set the filepos cursor
	int32_t fauxPtr = wadPtrStart + 12 + (sizeof(directory_t) * lump_count);
	bool insideSprites = false;
	for (WADEntry *node = entries; node; node = (WADEntry *)node->next)
	{
		int32_t padding_size = ((node->GetDataLength() - 1) & 3) ^ 3;

		if (!strcmp(node->GetName(), "S_START"))
			insideSprites = true;
		if (!strcmp(node->GetName(), "S_END"))
			insideSprites = false;

		if (bankSwitching)
		{
			int fauxPtrHalfMeg = fauxPtr / HALFMEG;
			if (insideSprites)
			{
				// If we're looking at sprites, we need to ensure the header entry, as well as the data entry,
				// stay in the same 512kb bank.
				if (strcmp(node->GetName(), ".")) // Header entry (i.e., PLAYA1)
				{
					WADEntry *dataEntry = (WADEntry *)node->next;
					int32_t dataEntry_padding_size = ((dataEntry->GetDataLength() - 1) & 3) ^ 3;

					int32_t endOfPair = fauxPtr + node->GetDataLength() + padding_size + dataEntry->GetDataLength() + dataEntry_padding_size;

					if (fauxPtrHalfMeg != (endOfPair / HALFMEG))
						fauxPtr = ((fauxPtrHalfMeg + 1) * HALFMEG);
				}
			}
			else if (fauxPtrHalfMeg != (fauxPtr + node->GetDataLength() + padding_size) / HALFMEG)
					fauxPtr = ((fauxPtrHalfMeg + 1) * HALFMEG);
		}

		*node->dir_entry_filepos = swap_endian32(fauxPtr);
		fauxPtr += node->GetDataLength();
		fauxPtr += padding_size;
	}

	fwrite("IWAD", 1, 4, f);
	uint32_t swapped_lump_count = swap_endian32(lump_count);
	fwrite(&swapped_lump_count, 4, 1, f);
	uint32_t swapped_table_ptr = swap_endian32((uint32_t)0xC);
	fwrite(&swapped_table_ptr, 4, 1, f);

	fwrite(directory, sizeof(directory_t), lump_count, f);

	int32_t bytesWritten = 12 + sizeof(directory_t) * lump_count;

	// Now we just spill all of the data
	for (WADEntry *node = entries; node; node = (WADEntry *)node->next)
	{
		size_t thetell = ftell(f);
		*node->dir_entry_filepos = swap_endian32(thetell);

		if (node->GetDataLength() > 0)
		{
			fwrite(node->GetData(), node->GetDataLength(), 1, f);
			bytesWritten += node->GetDataLength();
		}

		// We need to maintain 4-byte alignments. Pad out the data with zeros as needed.
		int32_t padding_size = ((node->GetDataLength() - 1) & 3) ^ 3;
		for (int i=0; i < padding_size; i++)
			fputc(0, f);

		bytesWritten += padding_size;

		if (node->next)
		{
			int32_t nextpos = swap_endian32(*((WADEntry *)node->next)->dir_entry_filepos);
			int32_t numbytesToPad = nextpos - bytesWritten - wadPtrStart;
			for (int i = 0; i < numbytesToPad; i++)
			{
				fputc(0, f);
				bytesWritten++;
			}
		}
	}

	fseek(f, 12, SEEK_SET);
	fwrite(directory, sizeof(directory_t), lump_count, f);
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
					mt->masked = 0;
					// Check this texture if it contains any transparent pixels.
					// If so, we need to set masked to 1.
					if (node->IsCompressed())
					{
						byte *decomp = node->Decompress();
						if (ContainsPixel(decomp, mt->width, mt->height, 0) || !strcmp(entryName, "GFZDOOR") || !strcmp(entryName, "GFZWINDP"))
							mt->masked = 1;
						free(decomp);
					}
					else
					{
						if (ContainsPixel(node->GetData(), mt->width, mt->height, 0) || !strcmp(entryName, "GFZDOOR") || !strcmp(entryName, "GFZWINDP"))
							mt->masked = 1;
					}
				}
			}
		}
	}

	int32_t newLumpLength;
	byte *newLump = t1->CreateLump(&newLumpLength);
	texture1->SetData(newLump, newLumpLength);
}

Exporter_Jaguar::Exporter_Jaguar(WADEntry *entries, FILE *f, int wadPtrStart)
{
	this->entries = entries;
	this->f = f;
	this->wadPtrStart = wadPtrStart;
}

Exporter_Jaguar::~Exporter_Jaguar()
{
	// Any Cleanup?
	fclose(f);
}
