#include "SRB2LevelConv.h"
#include <string.h>
#include <stdio.h>

typedef struct
{
	const char *oldName;
	const char *newName;
} remapName_t;

const remapName_t remapWalls[] = {

	{"-", "-" },
	// GFZ
	{ "GFZROCK", "GFZROCK"},
	{ "GFZROCKB", "GFZROCK" },
	{ "GFZROCKC", "GFZROCK" },
	{ "GFZBLOCK", "GFZBLOCK"},
	{ "GFZBLOKS", "GFZBLOCK"},
	{ "GFZBRID2", "GFZBRIDG"},
	{ "GFZBRIDG", "GFZBRIDG"},
	{ "GFZCRACK", "GFZCRACK"},
	{ "GFZCRAK1", "GFZCRACK"},
	{ "GFZCRAK2", "GFZCRACK"},
	{ "GFZFENC2", "GFZFNCD"},
	{ "GFZFENCW", "GFZFNCC"},
	{ "GFZFNCB", "GFZFA0"},
	{ "GFZFNCC", "GFZFNCC"},
	{ "GFZFNCCG", "GFZFNCC"},
	{ "GFZFNCD", "GFZFNCD"},
	{ "GFZGRASS", "GFZGA0"},
	{ "GFZGRSW", "GFZGRSW"},
	{ "GFZINSID", "GFZINSID"},
	{ "GFZPLTF", "GFZROCK"},
	{ "GFZRAIL", "GFZRA0"},
	{ "GFZRAIL2", "GFZRA0"},
	{ "GFZRAIL3", "GFZRA0"},
	{ "GFZROOF", "GFZROCK"},
	{ "GFZVINE1", "GFZROCK"},
	{ "GFZVINE2", "GFZROCK" },
	{ "GFZVINE3", "GFZROCK" },
	{ "GFZWALL", "GFZROCK" },
	{ "GFZWALL2", "GFZROCK" },
	{ "GFZWALLB", "GFZROCK" },
	{ "GFZWINDW", "GFZBRIK1" },
	{ "GFALL1", "GFALL1" },
	{ "GFALL2", "GFALL2" },
	{ "GFALL3", "GFALL3" },
	{ "GFALL4", "GFALL4" },
	{ "CFALL1", "GFALL1" },
	{ "CFALL2", "GFALL2" },
	{ "CFALL3", "GFALL3" },
	{ "CFALL4", "GFALL4" },
	{ "JNGWD4", "GFZWOOD" },
	{ NULL, NULL},
};

const remapName_t remapFlats[] = {
	// GFZ
	{ "F_SKY1", "F_SKY1" },
	{ "FWATER1", "BWATER01" },
	{ "GFZFLR01", "GFZFLR01" },
	{ "GFZFLR02", "GFZFLR02" },
	{ "GFZFLR03", "GFZFLR03" },
	{ "GFZFLR05", "GFZFLR05" },
	{ "GFZFLR06", "GFZFLR06" },
	{ "GFZFLR10", "GFZFLR10" },
	{ "GFZFLR11", "GFZFLR11" },
	{ "GFZFLR14", "GFZFLR01" },
	{ "GFZFLR15", "GFZFLR01" },
	{ "GFZFLR17", "GFZFLR01" },
	{ "GFZFLR18", "GFZFLR05" },
	{ "GFZFLR19", "GFZFLR06" },
	{ "GFZFLR21", "GFZFLR10" },
	{ "GFZFLR22", "GFZFLR10" },
	{ "GFZFNCF", "GFZFNCF" },
	{ "GFZFNCG", "GFZFNCF" },


	{ "TRAPFLR", "TRAPFLR" },
	{ "YELFLR", "YELFLR" },
	{ NULL, NULL },

};

void CheckReplacementWall(char *srcData)
{
	char srcDataText[9];
	memset(srcDataText, 0, sizeof(srcDataText));
	for (int32_t i = 0; i < 8; i++)
		srcDataText[i] = srcData[i];

	const remapName_t *remap = remapWalls;

	while (remap->oldName)
	{
		if (!strcmp(remap->oldName, srcDataText))
		{
			memset(srcDataText, 0, sizeof(srcDataText));
			strcpy(srcDataText, remap->newName);
			for (int32_t i = 0; i < 8; i++)
				srcData[i] = srcDataText[i];
			return;
		}

		remap++;
	}

	// Got here? Default to GFZROCK
	printf("Unknown wall %s, replacing with GFZROCK.\n", srcDataText);
	memset(srcDataText, 0, sizeof(srcDataText));
	strcpy(srcDataText, "GFZROCK");
	for (int32_t i = 0; i < 8; i++)
		srcData[i] = srcDataText[i];
}

void CheckReplacementFlat(char *srcData)
{
	char srcDataText[9];
	memset(srcDataText, 0, sizeof(srcDataText));
	for (int32_t i = 0; i < 8; i++)
		srcDataText[i] = srcData[i];

	const remapName_t *remap = remapFlats;

	while (remap->oldName)
	{
		if (!strcmp(remap->oldName, srcDataText))
		{
			memset(srcDataText, 0, sizeof(srcDataText));
			strcpy(srcDataText, remap->newName);
			for (int32_t i = 0; i < 8; i++)
				srcData[i] = srcDataText[i];
			return;
		}

		remap++;
	}

	// Got here? Default to GFZFLR01
	printf("Unknown flat %s, replacing with GFZFLR01.\n", srcDataText);
	memset(srcDataText, 0, sizeof(srcDataText));
	strcpy(srcDataText, "GFZFLR01");
	for (int32_t i = 0; i < 8; i++)
		srcData[i] = srcDataText[i];
}

WADEntry *ConvertSRB2Map(WADMap *map)
{
	for (int32_t i = 0; i < map->numsidedefs; i++)
	{
		sidedef_t *sidedef = &map->sidedefs[i];

		CheckReplacementWall(sidedef->toptexture);
		CheckReplacementWall(sidedef->midtexture);
		CheckReplacementWall(sidedef->bottomtexture);
	}

	for (int32_t i = 0; i < map->numsectors; i++)
	{
		sector_t *sector = &map->sectors[i];

		CheckReplacementFlat(sector->floorpic);
		CheckReplacementFlat(sector->ceilingpic);
	}

	return map->CreatePC(map->name);
}