#include "Exporter_Jaguar.h"
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





/*	int lump;

	conversion_task = PC_2_MARS;

	fseek(in_file, 0, SEEK_END);
	file_size = ftell(in_file);
	rewind(in_file);

	char label[5];
	label[4] = '\0';

	// Read the file signature to verify the supplied file is an IWAD or PWAD.
	fread(label, 4, 1, in_file);
	if (!strcmp(label, "IWAD") || !strcmp(label, "PWAD"))
	{
		// "DAWI" or "DAWP" found
		fread(&lump_count, 4, 1, in_file);
		fread(&table_ptr, 4, 1, in_file);
		if (table_ptr < 0xC || table_ptr > file_size)
		{
			printf("ERROR: WAD data not found.\n");
			return;
		}
	}
	else
	{
		// "IWAD" or "PWAD" not found
		printf("ERROR: WAD data not found.\n");
		return;
	}

	out_table = (byte *)malloc(1);

	lump_count += 3; // fix the patch count

	data = (byte *)malloc(table_ptr - 0xC);
	fread(data, 1, table_ptr - 0xC, in_file);
	table = (byte *)malloc(lump_count * 16);
	for (int i = 0; i < lump_count; i++)
	{
		fread(&table[(i * 16)], 4, 1, in_file);		// ptr
		*(int *)&table[(i * 16)] = swap_endian32(*(int *)&table[(i * 16)]);

		fread(&table[(i * 16) + 4], 4, 1, in_file);		// size
		*(int *)&table[(i * 16) + 4] = swap_endian32(*(int *)&table[(i * 16) + 4]);

		fread(&table[(i * 16) + 8], 1, 8, in_file);		// name
	}

	for (int i = 0; i < lump_count; i++)
	{
		*(int *)&table[i * 16] = swap_endian32(*(int *)&table[i * 16]);
		*(int *)&table[(i * 16) + 4] = swap_endian32(*(int *)&table[(i * 16) + 4]);

		// Set PLAYPAL to PLAYPALS
		// TODO: Jaguar does not have a 'PLAYPALS'
		if (!memcmp(&table[(i * 16) + 8], "PLAYPAL", strlen("PLAYPAL")))
			table[(i * 16) + 15] = 'S';
	}
	fclose(in_file);

#ifdef IMPORT_MODE_WAD
	strcpy(label, "DAWI"); // make sure we're creating an IWAD
	fwrite(&label, 4, 1, out_file);
	fseek(out_file, 8, SEEK_CUR);
	iwad_ptr = 0;
#else
	fseek(out_file, 0, SEEK_END);
	out_file_size = ftell(out_file);
	rewind(out_file);

	// Check for "DAWI" marker at start of file
	fread(&label, 4, 1, out_file);
	if (!strcmp(label, "DAWI"))
	{
		// "DAWI" found
		fread(&out_lump_count, 4, 1, out_file);
		out_lump_count = swap_endian32(out_lump_count);
		if (out_lump_count > 0 && out_lump_count <= 2048)
		{
			// limit based on "MAXLUMPS" in Jaguar source
			fread(&out_table_ptr, 4, 1, out_file);
			out_table_ptr = swap_endian32(out_table_ptr);
			if (out_table_ptr >= 0xC && out_table_ptr < 0x400000)
			{
				// limit of 4MB based on maximum Genesis ROM size
				iwad_ptr = 4; // The position right after DAWI in the file
				fseek(out_file, 4 + 0xC, SEEK_SET);	// required because this is using "r+b"
				out_file_size = 0;
			}
		}
	}
	else
	{
		// "DAWI" not found"
		printf("ERROR: WAD data not found.\n");
		system("PAUSE");
		return;
	}
#endif

	out_file_size = 0xC;
	out_lump_count = lump_count;

	for (lump = 0; lump < lump_count - 3; lump++)
	{
		char entryName[9];

		WADEntry *entry = new WADEntry();
		entry->SetIsCompressed(SetEntryName(entryName, (const char *)&table[(lump * 16) + 8]));
		entry->SetName(entryName);
		Listable::Add(entry, (Listable **)&wadEntries);
		// TODO: Add data via entry->SetData()

		printf("0x%04X\t%s\n", lump + lump_value_fix, entryName);
		switch (type)
		{
		case TYPE_TEXTURE:
			if (!strcmp(entryName, "P_END"))
			{
				WriteTableEnd(TYPE_TEXTURE);
				type = 0;
				if (floor_end_written)
				{
					lump_value_fix++;
					printf("0x%04X\tTEXTURE1\n", lump + lump_value_fix);
					CreateTEXTURE1(lump);
				}
			}
			else if (IsInExclusionList(entryName))
			{
				lump_value_fix--;
			}
			else
			{
				ConvertTextureData32X(lump);
			}
			break;
		case TYPE_FLOOR:
			if (!strcmp(entryName, "F_END"))
			{
				WriteTableEnd(TYPE_FLOOR);
				type = 0;
				if (texture_end_written)
				{
					lump_value_fix++;
					printf("0x%04X\tTEXTURE1\n", lump + lump_value_fix);
					CreateTEXTURE1(lump);
				}
			}
			else if (IsInExclusionList(entryName))
			{
				lump_value_fix--;
			}
			else
				ConvertFloorData(lump);
			break;
		case TYPE_SPRITE:
			if (!strcmp(entryName, "S_END"))
			{
				//WriteTableEnd(TYPE_SPRITE);
				lump_value_fix--;
				type = 0;
			}
			else
			{
				//TODO: ConvertSpriteDataFromPCToJag(lump);
				lump_value_fix++;
				out_lump_count++;
			}
			break;
		default:
			if (!strcmp(entryName, "P_START"))
			{
				WriteTableStart(TYPE_TEXTURE);
				type = TYPE_TEXTURE;
			}
			else if (!strcmp(entryName, "F_START"))
			{
				WriteTableStart(TYPE_FLOOR);
				type = TYPE_FLOOR;
			}
			else if (!strcmp(entryName, "S_START"))
			{
				//WriteTableStart(TYPE_SPRITE);
				lump_value_fix--;
				type = TYPE_SPRITE;
			}
			else if (!strcmp(entryName, "_32X_"))
			{
				Read_32X_(lump);
				lump_value_fix--;
			}
			else if (!strcmp(entryName, "COLORMAP"))
			{
				ConvertCOLORMAPData32X(lump);
			}
			else if (!strcmp(entryName, "PLAYPALS"))
			{
				ConvertPLAYPALData(lump);	// used for both PC and 32X conversions
			}
			else if (IsInExclusionList(entryName))
			{
				lump_value_fix--;
			}
			else if (!strcmp(entryName, "THINGS"))
			{
				ConvertMapData32X(lump, THINGS);
			}
			else if (!strcmp(entryName, "LINEDEFS"))
			{
				ConvertMapData32X(lump, LINEDEFS);
			}
			else if (!strcmp(entryName, "SIDEDEFS"))
			{
				ConvertMapData32X(lump, SIDEDEFS);
			}
			else if (!strcmp(entryName, "VERTEXES"))
			{
				ConvertMapData32X(lump, VERTEXES);
			}
			else if (!strcmp(entryName, "SEGS"))
			{
				ConvertMapData32X(lump, SEGS);
			}
			else if (!strcmp(entryName, "SSECTORS"))
			{
				ConvertMapData32X(lump, SSECTORS);
			}
			else if (!strcmp(entryName, "NODES"))
			{
				ConvertMapData32X(lump, NODES);
			}
			else if (!strcmp(entryName, "SECTORS"))
			{
				ConvertMapData32X(lump, SECTORS);
			}
			else if (!strcmp(entryName, "REJECT"))
			{
				ConvertMapData32X(lump, REJECT);
			}
			else if (!strcmp(entryName, "BLOCKMAP"))
			{
				ConvertMapData32X(lump, BLOCKMAP);
			}
			else
				ConvertRawData(lump);
			break;
		}
	}

	out_lump_count -= 7;
#ifndef FORCE_LITTLE_ENDIAN
	int i = swap_endian32(ftell(out_file) - iwad_ptr);
	out_lump_count = swap_endian32(out_lump_count);
#else
	i = ftell(out_file) - iwad_ptr;
#endif

	fwrite(out_table, 1, out_table_size, out_file);

#ifndef IMPORT_MODE_WAD
	printf("\n");

	// Adjust ROM size
	int n = ftell(out_file);
	printf("ROM size = %i.%iMB\n", n >> 20, (int)(((float)((n >> 16) & 0xF) / 16.0f) * 10));
	while ((n & 0xFFFFF) != 0)
	{
		fputc(0xFF, out_file);
		n++;
	}
	fseek(out_file, 0x1A4, SEEK_SET);
	n--;
	n = swap_endian32(n);
	fwrite(&n, 4, 1, out_file);
	n = swap_endian32(n);
	n++;
	printf("File size = %i.%iMB\n", n >> 20, (int)(((float)((n >> 16) & 0xF) / 16.0f) * 10));
	if (n > 0x400000)
		printf("WARNING: ROM exceeds the Sega Genesis ROM size limitation of 4MB.\n");
#endif

	fseek(out_file, iwad_ptr + 4, SEEK_SET);
	fwrite(&out_lump_count, 4, 1, out_file);
	fwrite(&i, 4, 1, out_file);

#ifndef IMPORT_MODE_WAD
	// Fix checksum
	checksum = 0;
	fseek(out_file, 0x200, SEEK_SET);
	for (t = 0x200; t < n; t += 2)
	{
		fread(&d, 2, 1, out_file);
		d = (swap_endian32(d) >> 16);
		checksum += d;
	}
	fseek(out_file, 0x18E, SEEK_SET);
	checksum = (swap_endian32(checksum) >> 16);
	fwrite(&checksum, 2, 1, out_file);
	checksum = (swap_endian32(checksum) >> 16);
	printf("Checksum = 0x%04X\n", checksum);
#endif

	printf("\nImport finished!\n");
	return;*/
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
