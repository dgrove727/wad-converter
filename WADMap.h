#pragma once
#include <stdint.h>
#include "WADEntry.h"

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

struct WADMap
{
private:
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
	int16_t numsegs;
	int16_t numsubsectors;
	int16_t numnodes;
	int16_t numsectors;
	size_t rejectSize;
	size_t blockmapSize;

	void CompressSidedefs();

	WADEntry *CreateJaguar(const char *mapname);
	WADEntry *CreatePC(const char *mapname);

	WADMap(WADEntry *head);
	virtual ~WADMap();
};

