#pragma once
#include <stdint.h>
#include "WADEntry.h"
#include "Texture1.h"
#include "FlatList.h"

#define LOADFLAGS_VERTEXES 1
#define LOADFLAGS_BLOCKMAP 2
#define LOADFLAGS_REJECT 4
#define LOADFLAGS_NODES 8
#define LOADFLAGS_SEGS 16
#define LOADFLAGS_LINEDEFS 32
#define LOADFLAGS_SUBSECTORS 64
#define LOADFLAGS_SECTORS 128

typedef struct
{
	int16_t x;
	int16_t y;
	int16_t angle;
	int16_t type;
	int16_t options;
} mapthing_t;

typedef struct
{
	int16_t v1;
	int16_t v2;
	int16_t flags;
	int16_t special;
	int16_t tag;
	int16_t sidenum[2];
} linedef_t;

typedef struct
{
	int16_t textureoffset;
	int16_t rowoffset;
	char toptexture[8];
	char bottomtexture[8];
	char midtexture[8];
	int16_t sector;
} sidedef_t;

typedef struct
{
	int16_t x;
	int16_t y;
} vertex_t;

typedef struct
{
	int32_t x;
	int32_t y;
} jagVertex_t;

typedef struct
{
	int16_t v1;
	int16_t v2;
	int16_t angle;
	int16_t linedef;
	int16_t side;
	int16_t offset;
} seg_t;

typedef struct
{
	int16_t numsegs;
	int16_t firstseg;
} subsector_t;

typedef struct
{
	int16_t x;
	int16_t y;
	int16_t dx;
	int16_t dy;
	int16_t bbox[2][4];
	uint16_t children[2];
} node_t;

typedef struct
{
	int32_t x;
	int32_t y;
	int32_t dx;
	int32_t dy;
	int32_t bbox[2][4];
	int32_t children[2];
} jagnode_t;

typedef struct
{
	int16_t floorheight;
	int16_t ceilingheight;
	char floorpic[8];
	char ceilingpic[8];
	int16_t lightlevel;
	int16_t special;
	int16_t tag;
} sector_t;

typedef struct
{
	int32_t floorheight, ceilingheight;
	int16_t specialdata;
	uint8_t floorpic, ceilingpic;
	uint8_t lightlevel, special;
	uint8_t tag;
	uint8_t flags;
	uint16_t floor_offs;
	int16_t heightsec;
	int16_t fofsec;
	int16_t specline;
	int16_t extrasecdata;
} staticsector_t;

struct WADMap
{
private:
	void UpdateSectorSidedefRefs(int16_t oldSectornum, int16_t newSectornum);
	void UpdateLinedefSidedefRefs(int16_t oldSidenum, int16_t newSidenum);

public:
	char *name;

	mapthing_t *things;
	linedef_t *linedefs;
	sidedef_t *sidedefs;
	vertex_t *vertexes;
	seg_t *segs;
	subsector_t *subsectors;
	node_t *nodes;
	jagnode_t *jagNodes;
	sector_t *sectors;
	uint8_t *reject;
	uint8_t *blockmap;

	int16_t numthings;
	int16_t numlinedefs;
	int16_t numsidedefs;
	int16_t numvertexes;
	uint16_t numsegs;
	int16_t numsubsectors;
	int16_t numnodes;
	int16_t numsectors;
	int32_t rejectSize;
	int32_t blockmapSize;

	int16_t GetSectorFromSeg(int16_t firstline);
	bool SectorsAreIdentical(sector_t *src, sector_t *cmp);
	int16_t GetFrontsector(linedef_t *line);
	int16_t GetSectorWithTag(staticsector_t *fullSectors, int16_t start, uint8_t tag);

	void DefragTags();

	void CompressSidedefs();

	WADEntry *CreateJaguar(const char *mapname, int loadFlags, bool srb32xsegs = false, Texture1 *t1 = NULL, FlatList *fList = NULL);
	WADEntry *CreatePC(const char *mapname);

	WADMap(WADEntry *head);
	virtual ~WADMap();
};

