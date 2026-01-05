#include "WADMap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#define DYNAMIC_SECTORS

#define SF_FOF_INVISIBLE_TANGIBLE    1
#define SF_FOF_SWAPHEIGHTS           2
#define SF_FLOATBOB                  4
#define SF_AIRBOB                    8
#define SF_CRUMBLE                  16
#define SF_RESPAWN                  32
#define SF_FOF_CONTROLSECTOR        64
#define SF_CONVEYOR            128

#define	ML_BLOCKING			1
#define	ML_BLOCKMONSTERS	2
#define ML_HAS_SPECIAL_OR_TAG 4 // Have to reference hash tables to find special/tag

/* if a texture is pegged, the texture will have the end exposed to air held */
/* constant at the top or bottom of the texture (stairs or pulled down things) */
/* and will move with a height change of one of the neighbor sectors */
/* Unpegged textures allways have the first row of the texture at the top */
/* pixel of the line for both top and bottom textures (windows) */
#define	ML_DONTPEGTOP		8
#define	ML_DONTPEGBOTTOM	16
#define ML_CULLING			32	/* Cull this line by distance */
#define ML_NOCLIMB			64
#define ML_UNDERWATERONLY  128 /* Cull line if view is out of water */
#define ML_CULL_MIDTEXTURE  256
#define	ML_MIDTEXTUREBLOCK  512	/* Collide with midtexture (fences, etc.) */

#define ML_UNUSED2_WRAPMIDTEX 1024 // TODO: I like this idea...

/* to aid move clipping */
#define ML_ST_HORIZONTAL 	4096 // Not used
#define ML_ST_VERTICAL	 	8192 // Not used
#define ML_ST_POSITIVE	 	16384
#define ML_ST_NEGATIVE	 	32768

typedef struct
{
	int16_t v1, v2;
	int16_t sideoffset;
	int16_t linedef;
} srb32xseg_t;

typedef struct
{
	int16_t x, y, dx, dy; // partition line
	uint16_t children[2]; // if NF_SUBSECTOR, it's a subsector
	uint16_t encbbox[2]; // encoded bounding box for each child
} srb32xnode_t;

typedef struct
{
	int16_t v1;
	int16_t v2;
	int16_t sidenum[2];
} srb32xlinedef_t;

typedef struct
{
	uint16_t flags;
	uint8_t special;
	uint8_t tag;
} srb32xldflags_t;

typedef struct
{
	int16_t sector;
	uint8_t texIndex;
	uint8_t rowoffset;     // add this to the calculated texture top
	int16_t textureoffset; // 8.4, add this to the calculated texture col
} srb32xsidedef_t;

typedef struct
{
	int16_t		floorheight, ceilingheight;
	uint8_t		floorpic, ceilingpic;
	uint8_t     lightlevel;
	uint8_t     special, tag;
} srb32xsector_t;

typedef struct
{
	staticsector_t *sector;
	float sort_value;
	int original_index;
} sortablesector_t;

typedef struct
{
	int16_t firstline;
	int16_t isector;
} srb32xsubsector_t;

typedef struct
{
	uint8_t top;
	uint8_t mid;
	uint8_t bottom;
} sidetex_t;

static float GetSectorSortValue(staticsector_t *sector, bool hasLightning)
{
	if (sector->tag == 0 && hasLightning && (sector->ceilingpic == 0xff || sector->floorpic == 0xff))
		return 0.5f;

	if (sector->tag == 0 || sector->specialdata > 0) // specialdata > 0 means it's a static FOF, water, etc.
		return 0.0f;

	return (float)sector->tag;
}

static void insertion_sort_by_value(sortablesector_t *items, size_t n)
{
	for (size_t i = 1; i < n; ++i) {
		sortablesector_t key = items[i];
		size_t j = i;

		while (j > 0 && items[j - 1].sort_value > key.sort_value) {
			items[j] = items[j - 1];
			--j;
		}
		items[j] = key;
	}
}

void SortAndRemapSectors(staticsector_t *sectors, size_t num_sectors,
	sidedef_t *sides, size_t num_sides,
	bool hasLightning)
{
	// Build temporary array with sort keys and original indices
	sortablesector_t *temp = (sortablesector_t*)malloc(num_sectors * sizeof(sortablesector_t));
	
	for (size_t i = 0; i < num_sectors; ++i)
	{
		temp[i].sector = &sectors[i];
		temp[i].sort_value = GetSectorSortValue(&sectors[i], hasLightning);
		temp[i].original_index = (int)i;
	}

	// Sort the temporary array by sort_value (stable insertion sort)
	insertion_sort_by_value(temp, num_sectors);

	// Build old -> new mapping and reorder the sectors array
	int *old_to_new = (int*)malloc(num_sectors * sizeof(int));
	staticsector_t *reordered = (staticsector_t*)malloc(num_sectors * sizeof(staticsector_t));

	for (size_t new_pos = 0; new_pos < num_sectors; ++new_pos)
	{
		int old_pos = temp[new_pos].original_index;
		old_to_new[old_pos] = (int)new_pos;
		reordered[new_pos] = *temp[new_pos].sector;  // copy full struct
	}

	// Copy reordered sectors back into original array
	for (size_t i = 0; i < num_sectors; ++i)
		sectors[i] = reordered[i];

	// Remap all sidedef_t references
	for (size_t i = 0; i < num_sides; ++i) {
		int old_idx = sides[i].sector;
		if (old_idx >= 0 && (size_t)old_idx < num_sectors)
			sides[i].sector = old_to_new[old_idx];
	}

	free(reordered);
	free(old_to_new);
	free(temp);
}


#define LOADFLAGS_VERTEXES 1
#define LOADFLAGS_BLOCKMAP 2
#define LOADFLAGS_REJECT 4
#define LOADFLAGS_NODES 8
#define LOADFLAGS_SEGS 16
#define LOADFLAGS_LINEDEFS 32
#define LOADFLAGS_SUBSECTORS 64
#define LOADFLAGS_SECTORS 128

static uint8_t RemapLinedefSpecial(int16_t special)
{
	switch (special)
	{
	case 300:
		return 230;
	case 301:
		return 231;
	case 302:
		return 232;
	case 407:
		return 222;
	case 406: // Focus on a point
		return 221;
	case 405:
		return 220;
	case 404:
		return 219;
	case 403:
		return 218;
	default:
		break;
	}

	return (uint8_t)special;
}

static uint8_t FindFlat(FlatList *fList, char name[8])
{
	char checkName[9];
	memcpy(checkName, name, 8);
	checkName[8] = '\0';

	if (!strcmp(checkName, "F_SKY1"))
		return 0xff;

	FlatList *node;
	uint8_t count = 0;
	for (node = fList; node; node = (FlatList *)node->next)
	{
		if (!strcmp(node->name, checkName))
			return count;

		count++;
	}

	return 0;
}

static uint8_t FindTexture(Texture1 *t1, char name[8])
{
	char checkName[9];
	memcpy(checkName, name, 8);
	checkName[8] = '\0';

	if (name[0] == '-' || (name[0] == 'C' && name[1] == 'E' && name[2] == 'Z' && name[3] == 'T' && name[4] == 'R' && name[5] == 'A' && name[6] == '0'))
		return 0;

	MapTexture *node;
	uint8_t count = 0;
	for (node = t1->mapTextures; node; node = (MapTexture *)node->next)
	{
		if (!strcmp(node->name, checkName))
			return count;

		count++;
	}

	return 0;
}

int16_t worldbbox[4];

static srb32xnode_t *nodes;
static int numnodes;

#define NF_SUBSECTOR 0x8000

enum { BOXTOP, BOXBOTTOM, BOXLEFT, BOXRIGHT };	/* bbox coordinates */

static int P_EncodeBBoxSide(int16_t *b, int16_t *outerbbox, int pc, int nc)
{
	int length, unit;
	int nu, pu;

	length = outerbbox[pc] - outerbbox[nc] + 15;
	if (length < 16)
		return 0;
	unit = length / 16;

	// negative corner is increasing
	nu = 0;
	length = b[nc] - outerbbox[nc];
	if (length > 0) {
		nu = length / unit;
		if (nu > 15)
			nu = 15;
		b[nc] = outerbbox[nc] + nu * unit;
	}

	// positive corner is decreasing
	pu = 0;
	length = outerbbox[pc] - b[pc];
	if (length > 0) {
		pu = length / unit;
		if (pu > 15)
			pu = 15;
		b[pc] = outerbbox[pc] - pu * unit;
	}

	return (pu << 4) | nu;
}

// encodes bbox as the number of 1/16th units of parent bbox on each side
static int P_EncodeBBox(int16_t *cb, int16_t *outerbbox)
{
	int encbbox;
	encbbox = P_EncodeBBoxSide(cb, outerbbox, BOXRIGHT, BOXLEFT);
	encbbox <<= 8;
	encbbox |= P_EncodeBBoxSide(cb, outerbbox, BOXTOP, BOXBOTTOM);
	return encbbox;
}

static void P_EncodeNodeBBox_r(int nn, int16_t *bboxes, int16_t *outerbbox)
{
	int 		j;
	srb32xnode_t *n;

	if (nn & NF_SUBSECTOR)
		return;

	n = nodes + nn;
	for (j = 0; j < 2; j++)
	{
		int16_t *bbox = &bboxes[nn * 8 + j * 4];
		n->encbbox[j] = P_EncodeBBox(bbox, outerbbox);
		P_EncodeNodeBBox_r(n->children[j], bboxes, bbox);
	}
}

// set the world's bounding box
// recursively encode bounding boxes for all BSP nodes
static void P_EncodeNodesBBoxes(int16_t *bboxes)
{
	int j;

	worldbbox[BOXLEFT] = INT16_MAX;
	worldbbox[BOXRIGHT] = INT16_MIN;
	worldbbox[BOXBOTTOM] = INT16_MAX;
	worldbbox[BOXTOP] = INT16_MIN;

	for (j = 0; j < 2; j++)
	{
		int16_t *cb = &bboxes[(numnodes - 1) * 8 + j * 4];
		if (cb[BOXLEFT] < worldbbox[BOXLEFT])
			worldbbox[BOXLEFT] = cb[BOXLEFT];
		if (cb[BOXRIGHT] > worldbbox[BOXRIGHT])
			worldbbox[BOXRIGHT] = cb[BOXRIGHT];
		if (cb[BOXBOTTOM] < worldbbox[BOXBOTTOM])
			worldbbox[BOXBOTTOM] = cb[BOXBOTTOM];
		if (cb[BOXTOP] > worldbbox[BOXTOP])
			worldbbox[BOXTOP] = cb[BOXTOP];
	}

	P_EncodeNodeBBox_r(numnodes - 1, bboxes, worldbbox);
}

static void R_DecodeBBox(int16_t *bbox, const int16_t *outerbbox, uint16_t encbbox)
{
	uint16_t l;

	l = outerbbox[BOXTOP] - outerbbox[BOXBOTTOM];
	bbox[BOXBOTTOM] = outerbbox[BOXBOTTOM] + ((l * (encbbox & 0x0f)) >> 4);
	bbox[BOXTOP] = outerbbox[BOXTOP] - ((l * (encbbox & 0xf0)) >> 8);

	encbbox >>= 8;
	l = outerbbox[BOXRIGHT] - outerbbox[BOXLEFT];
	bbox[BOXLEFT] = outerbbox[BOXLEFT] + ((l * (encbbox & 0x0f)) >> 4);
	bbox[BOXRIGHT] = outerbbox[BOXRIGHT] - ((l * (encbbox & 0xf0)) >> 8);
}

static srb32xnode_t *CompressNodes(node_t *data, int count)
{
	numnodes = count;
	nodes = (srb32xnode_t*)malloc(numnodes * sizeof(srb32xnode_t));
	int16_t *bboxes = (int16_t*)malloc(1024 * 256); // 256k

	node_t *mn = data;
	srb32xnode_t *no = (srb32xnode_t*)nodes;
	for (int i = 0; i < numnodes; i++, no++, mn++)
	{
		no->x = mn->x;
		no->y = mn->y;
		no->dx = mn->dx;
		no->dy = mn->dy;

		for (int j = 0; j < 2; j++)
		{
			no->children[j] = mn->children[j];
			for (int k = 0; k < 4; k++)
				bboxes[i * 8 + j * 4 + k] = mn->bbox[j][k];
		}
	}

	P_EncodeNodesBBoxes(bboxes);
	free(bboxes);

	// Now we gotta swap everything.
	no = (srb32xnode_t *)nodes;
	for (int i = 0; i < numnodes; i++, no++)
	{
		no->x = swap_endian16(no->x);
		no->y = swap_endian16(no->y);
		no->dx = swap_endian16(no->dx);
		no->dy = swap_endian16(no->dy);

		no->children[0] = swap_endian16(no->children[0]);
		no->children[1] = swap_endian16(no->children[1]);

		no->encbbox[0] = swap_endian16(no->encbbox[0]);
		no->encbbox[1] = swap_endian16(no->encbbox[1]);
	}

	return nodes;
}

bool IdenticalSectors(const sector_t *sec1, const sector_t *sec2)
{
	if (sec1 == sec2) // Itself
		return false;

	return !memcmp(sec1, sec2, sizeof(sector_t));
}

bool IdenticalSidedefs(const sidedef_t *side1, const sidedef_t *side2)
{
	if (side1->rowoffset != side2->rowoffset)
		return false;

	if (side1->textureoffset != side2->textureoffset)
		return false;

	if (side1->sector != side2->sector)
		return false;

	if (memcmp(side1->bottomtexture, side2->bottomtexture, 8))
		return false;

	if (memcmp(side1->midtexture, side2->midtexture, 8))
		return false;

	if (memcmp(side1->toptexture, side2->toptexture, 8))
		return false;

	return true;
}

void WADMap::UpdateSectorSidedefRefs(int16_t oldSectornum, int16_t newSectornum)
{
	for (int i = 0; i < numsidedefs; i++)
	{
		if (sidedefs[i].sector == oldSectornum)
			sidedefs[i].sector = newSectornum;
	}
}

void WADMap::UpdateLinedefSidedefRefs(int16_t oldSidenum, int16_t newSidenum)
{
	for (int i = 0; i < numlinedefs; i++)
	{
		if (linedefs[i].sidenum[0] == oldSidenum)
			linedefs[i].sidenum[0] = newSidenum;

		if (linedefs[i].sidenum[1] == oldSidenum)
			linedefs[i].sidenum[1] = newSidenum;
	}
}

void WADMap::CompressSidedefs()
{
	int32_t numNewSidedefs = 0;

	int *sidedefLinedefnum = (int*)malloc(numsidedefs * sizeof(int));

	for (int32_t i = 0; i < numsidedefs; i++)
	{
		if (sidedefs[i].bottomtexture[0] == '-'
			&& sidedefs[i].toptexture[0] == '-'
			&& sidedefs[i].midtexture[0] == '-')
		{
			sidedefs[i].rowoffset = 0;
			sidedefs[i].textureoffset = 0;
		}
	}

	for (int32_t i = 0; i < numsidedefs; i++)
	{
		for (int32_t j = 0; j < numlinedefs; j++)
		{
			if (linedefs[j].sidenum[0] == i
				|| linedefs[j].sidenum[1] == i)
			{
				sidedefLinedefnum[i] = j;
				break;
			}
		}
	}

	for (int32_t i = 0; i < numsidedefs; i++)
	{
		if (sidedefs[i].sector == -1)
			continue;

		linedef_t *ld = this->linedefs;
		for (int32_t j = numlinedefs; j--; ld++)
		{
			if (ld->sidenum[0] == i)
				ld->sidenum[0] = numNewSidedefs;

			if (ld->sidenum[1] == i)
				ld->sidenum[1] = numNewSidedefs;
		}

		linedef_t *line1 = &linedefs[sidedefLinedefnum[i]];

		for (int32_t j = i + 1; j < numsidedefs; j++)
		{
			if (sidedefs[j].sector == -1)
				continue;

			linedef_t *line2 = &linedefs[sidedefLinedefnum[j]];

			if (line1->special == 249 || line2->special == 249)
				continue;

			if (IdenticalSidedefs(&sidedefs[i], &sidedefs[j]))
			{
				// Find the linedefs that belong to this one
				ld = linedefs;
				for (int32_t k = numlinedefs; k--; ld++)
				{
					if (ld->sidenum[0] == j)
						ld->sidenum[0] = numNewSidedefs;

					if (ld->sidenum[1] == j)
						ld->sidenum[1] = numNewSidedefs;
				}
				sidedefs[j].sector = -1; // Flag for deletion
			}
		}
		numNewSidedefs++;
	}

	free(sidedefLinedefnum);

	sidedef_t *newSidedefs = (sidedef_t *)malloc(numNewSidedefs * sizeof(sidedef_t));

	// Copy the sidedefs to their new block of memory.
	int32_t z = 0;
	for (int32_t i = 0; i < numsidedefs; i++)
	{
		if (sidedefs[i].sector == -1)
			continue;

		memcpy(&newSidedefs[z++], &sidedefs[i], sizeof(sidedef_t));
	}

	free(sidedefs);
	sidedefs = newSidedefs;
	numsidedefs = numNewSidedefs;
}

uint8_t FindSidetexIndex(sidetex_t *sidetexes, int *numsidetexes, sidetex_t findSidetex)
{
	for (int i = 0; i < *numsidetexes; i++)
	{
		if (sidetexes[i].top == findSidetex.top
			&& sidetexes[i].mid == findSidetex.mid
			&& sidetexes[i].bottom == findSidetex.bottom)
		{
			// Found it!
			return (uint8_t)i;
		}
	}

	// If we got here, we didn't find it. So make a new one!
	sidetexes[*numsidetexes].top = findSidetex.top;
	sidetexes[*numsidetexes].mid = findSidetex.mid;
	sidetexes[*numsidetexes].bottom = findSidetex.bottom;
	
	uint8_t retVal = *numsidetexes;
	*numsidetexes = (*numsidetexes) + 1;

	return retVal;
}

// Comparison function for qsort
int compare_by_type(const void *a, const void *b) {
	const mapthing_t *thing_a = (const mapthing_t *)a;
	const mapthing_t *thing_b = (const mapthing_t *)b;

	// Sort in ascending order by type
	return (thing_a->type - thing_b->type);
}

int16_t WADMap::GetSectorFromSeg(int16_t firstline)
{
	seg_t* seg = &segs[firstline];

	int16_t sideoffset = seg->offset;
	int16_t side = seg->side;
	side &= 1;

	sideoffset <<= 1;
	sideoffset |= side;

	linedef_t* linedef = &linedefs[seg->linedef];
	sidedef_t* sidedef = &sidedefs[linedef->sidenum[sideoffset & 1]];

	return sidedef->sector;
}

bool WADMap::SectorsAreIdentical(sector_t *src, sector_t *cmp)
{
	if (memcmp(src, cmp, sizeof(sector_t)))
		return false;

	// Okay, they are identical, but is it a control sector of certain kinds?
	for (int i = 0; i < numlinedefs; i++)
	{
		linedef_t *line = &linedefs[i];
		sidedef_t *side = &sidedefs[line->sidenum[0]];

		if (side->sector != src - sectors)
			continue;

		if (line->special == 160) // Water bobbing)
			return false;
	}

	for (int i = 0; i < numlinedefs; i++)
	{
		linedef_t *line = &linedefs[i];
		sidedef_t *side = &sidedefs[line->sidenum[0]];

		if (side->sector != cmp - sectors)
			continue;

		if (line->special == 160) // Water bobbing)
			return false;
	}

	return true;
}

// Renumber unique integers sequentially
static int *renumber_sequential(int *numbers, int size) {
	// Step 1: Copy and sort (unique guaranteed)
	int *sorted = (int*)malloc(size * sizeof(int));
	for (int i = 0; i < size; i++) {
		sorted[i] = numbers[i];
	}

	// Step 2: Simple bubble sort
	for (int i = 0; i < size - 1; i++) {
		for (int j = 0; j < size - i - 1; j++) {
			if (sorted[j] > sorted[j + 1]) {
				int temp = sorted[j];
				sorted[j] = sorted[j + 1];
				sorted[j + 1] = temp;
			}
		}
	}

	int *result = (int*)malloc(size * sizeof(int));
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (numbers[i] == sorted[j]) {
				result[i] = j + 1;
				break;
			}
		}
	}

	free(sorted);
	return result;
}

static bool ExistsInArray(int *array, int value)
{
	for (int i = 0; i < 1024; i++)
	{
		if (array[i] == value)
			return true;
	}

	return false;
}

static bool IsDynamicSector(staticsector_t *sector, bool hasLightning)
{
	return GetSectorSortValue(sector, hasLightning) > 0.0f;
}

void WADMap::DefragTags()
{
	int existingTags[1024]; // No more than 1024, right?....RIGHT?
	int c = 0;
	memset(existingTags, 0, 1024 * sizeof(int));

	// First, ensure that every sector is tagged to a linedef
	for (int i = 0; i < numsectors; i++)
	{
		bool found = false;

		for (int j = 0; j < numlinedefs; j++)
		{
			if (linedefs[j].tag == sectors[i].tag)
			{
				found = true;
				break;
			}
		}

		if (!found)
			sectors[i].tag = 0;
	}

	// Now ensure that every linedef is tagged to a sector
	for (int i = 0; i < numlinedefs; i++)
	{
		bool found = false;

		for (int j = 0; j < numsectors; j++)
		{
			if (sectors[j].tag == linedefs[i].tag)
			{
				found = true;
				break;
			}
		}

		if (!found)
			linedefs[i].tag = 0;
	}

	// Scan sectors
	for (int i = 0; i < numsectors; i++)
	{
		if (sectors[i].tag <= 0)
			continue;

		if (!ExistsInArray(existingTags, sectors[i].tag))
			existingTags[c++] = sectors[i].tag;
	}

	// Scan linedefs
	for (int i = 0; i < numlinedefs; i++)
	{
		if (linedefs[i].tag <= 0)
			continue;

		if (!ExistsInArray(existingTags, linedefs[i].tag))
			existingTags[c++] = linedefs[i].tag;
	}

	printf("Number of unique tags is %d\n", c);

	int *result = renumber_sequential(existingTags, c);

	for (int i = 0; i < c; i++)
	{
		// Remap sectors
		for (int j = 0; j < numsectors; j++)
		{
			if (sectors[j].tag == existingTags[i])
				sectors[j].tag = result[i] | 32768;
		}

		// Remap linedefs
		for (int j = 0; j < numlinedefs; j++)
		{
			if (linedefs[j].tag == existingTags[i])
				linedefs[j].tag = result[i] | 32768;
		}
	}

	for (int i = 0; i < numsectors; i++)
		sectors[i].tag &= ~32768;

	for (int i = 0; i < numlinedefs; i++)
		linedefs[i].tag &= ~32768;

	free(result);

	printf("Sector printout:\n");
	for (int i = 0; i < numsectors; i++)
	{
		if (sectors[i].tag <= 0)
			continue;

		printf("%d\n", sectors[i].tag);
	}

	printf("Linedef printout:\n");
	for (int i = 0; i < numlinedefs; i++)
	{
		if (linedefs[i].tag <= 0)
			continue;

		printf("%d\n", linedefs[i].tag);
	}
}

int16_t WADMap::GetFrontsector(linedef_t *line)
{
	sidedef_t *side = &sidedefs[line->sidenum[0]];

	return side->sector;
}

int16_t WADMap::GetSectorWithTag(staticsector_t *fullSectors, int16_t start, uint8_t tag)
{
	for (int i = start + 1; i < numsectors; i++)
	{
		if (fullSectors[i].tag == tag)
			return i;
	}

	return -1;
}

WADEntry *WADMap::CreateJaguar(const char *mapname, int loadFlags, bool srb32xsegs, Texture1 *t1, FlatList *fList)
{
	WADEntry *head = NULL;

//	CompressSectors();
	CompressSidedefs();
#ifdef DYNAMIC_SECTORS
	bool hasLightning = false;

	for (int i = 0; i < numsectors; i++)
	{
		if (sectors[i].tag > 255)
		{
			printf("Sector %d has tag %d, which is > 255.\n", i, sectors[i].tag);
			sectors[i].tag = 0;
		}
	}

	staticsector_t *fullSectors = (staticsector_t *)malloc(sizeof(staticsector_t) * numsectors);
	for (int i = 0; i < numsectors; i++)
	{
		staticsector_t *sec = &fullSectors[i];

		sec->ceilingheight = sectors[i].ceilingheight << FRACBITS;
		sec->ceilingpic = FindFlat(fList, sectors[i].ceilingpic);
		sec->extrasecdata = 0;
		sec->flags = 0;
		sec->floorheight = sectors[i].floorheight << FRACBITS;
		sec->floorpic = FindFlat(fList, sectors[i].floorpic);
		sec->floor_offs = 0;
		sec->fofsec = -1;
		sec->heightsec = -1;
		sec->lightlevel = (uint8_t)sectors[i].lightlevel;
		sec->special = (uint8_t)sectors[i].special;
		sec->specialdata = 0;
		sec->specline = -1;
		sec->tag = (uint8_t)sectors[i].tag;
	}

	// Process line specials for fofsec/heightsec 
	for (int i = 0; i < numlinedefs; i++)
	{
		uint8_t tag = (uint8_t)linedefs[i].tag;
		int16_t front = GetFrontsector(&linedefs[i]);
		switch (linedefs[i].special)
		{
			case 100:
			{
				for (int s = -1; (s = GetSectorWithTag(fullSectors, s, tag)) >= 0;)
				{
					fullSectors[s].fofsec = front;
					fullSectors[s].specialdata = 1;
					fullSectors[front].specline = i;
					fullSectors[front].flags |= SF_FOF_CONTROLSECTOR;

					if (linedefs[i].flags & ML_BLOCKMONSTERS)
						fullSectors[s].flags |= SF_FOF_SWAPHEIGHTS;
				}
				break;
			}

			case 120:
			{
				for (int s = -1; (s = GetSectorWithTag(fullSectors, s, tag)) >= 0;)
				{
					fullSectors[s].heightsec = front;
					fullSectors[s].specialdata = 1;
					fullSectors[front].specline = i;
				}
				break;
			}
		}
	}

	SortAndRemapSectors(fullSectors, numsectors, sidedefs, numsidedefs, hasLightning);

	// Process line specials for fofsec/heightsec 
	for (int i = 0; i < numlinedefs; i++)
	{
		uint8_t tag = (uint8_t)linedefs[i].tag;
		int16_t front = GetFrontsector(&linedefs[i]);
		switch (linedefs[i].special)
		{
		case 100:
		{
			for (int s = -1; (s = GetSectorWithTag(fullSectors, s, tag)) >= 0;)
			{
				fullSectors[s].fofsec = front;
				fullSectors[s].specialdata = 1;
				fullSectors[front].specline = i;
				fullSectors[front].flags |= SF_FOF_CONTROLSECTOR;

				if (linedefs[i].flags & ML_BLOCKMONSTERS)
					fullSectors[s].flags |= SF_FOF_SWAPHEIGHTS;
			}
			break;
		}

		case 120:
		{
			for (int s = -1; (s = GetSectorWithTag(fullSectors, s, tag)) >= 0;)
			{
				fullSectors[s].heightsec = front;
				fullSectors[s].specialdata = 1;
				fullSectors[front].specline = i;
			}
			break;
		}
		}
	}
#endif

	// Header
	WADEntry *entry = new WADEntry();
	entry->SetName(mapname);
	Listable::Add(entry, (Listable **)&head);

	// THINGS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("THINGS");
	entry->SetIsCompressed(true);
	// Order by mapnum
	// Ideally, we want to order by MF_RINGMOBJ and then regular items.
	qsort(things, numthings, sizeof(mapthing_t), compare_by_type);
	entry->SetData((byte*)things, numthings * sizeof(mapthing_t));

	// LINEDEFS
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("LINEDEFS");
	entry->SetIsCompressed(loadFlags & LOADFLAGS_LINEDEFS);

	// A little bit of Q&A
	{
		// Check linedefs with a special but no tag
		for (int i = 0; i < numlinedefs; i++)
		{
			if (linedefs[i].special > 0 && linedefs[i].tag == 0)
				printf("Linedef %d has special (%d) but no tag.\n", i, linedefs[i].special);
		}

		// Check linedefs with a tag, but no special
		for (int i = 0; i < numlinedefs; i++)
		{
			if (linedefs[i].special == 0 && linedefs[i].tag != 0)
				printf("Linedef %d has tag (%d) but no special.\n", i, linedefs[i].tag);
		}

		for (int s = 0; s < numsectors; s++)
		{
			if (sectors[s].tag <= 0)
				continue;

			bool found = false;
			for (int i = 0; i < numlinedefs; i++)
			{
				if (linedefs[i].tag == sectors[s].tag)
				{
					found = true;
					break;
				}
			}

			if (!found)
				printf("Sector %d has tag (%d) but no linedef tagged to it.\n", s, sectors[s].tag);
		}

		// Find identical sectors
		for (int x = 0; x < numsectors; x++)
		{
			sector_t* srcSec = &sectors[x];
			for (int y = 0; y < numsectors; y++)
			{
				if (x == y)
					continue;

				sector_t* cmpSec = &sectors[y];

				if (SectorsAreIdentical(srcSec, cmpSec))
					printf("Sectors %d and %d are identical.\n", x, y);
			}
		}
	}

	if (srb32xsegs)
	{
		srb32xlinedef_t *compData = (srb32xlinedef_t *)malloc(numlinedefs * sizeof(srb32xlinedef_t));
		for (int i = 0; i < numlinedefs; i++)
		{
			compData[i].v1 = swap_endian16(linedefs[i].v1);
			compData[i].v2 = swap_endian16(linedefs[i].v2);
			compData[i].sidenum[0] = swap_endian16(linedefs[i].sidenum[0]);
			compData[i].sidenum[1] = swap_endian16(linedefs[i].sidenum[1]);
		}

		entry->SetData((byte *)compData, numlinedefs * sizeof(srb32xlinedef_t));
		free(compData);
	}
	else
	{
		entry->SetData((byte *)linedefs, numlinedefs * sizeof(linedef_t));
	}

	// LDFLAGS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("LDFLAGS");
	entry->SetIsCompressed(true);

	if (srb32xsegs)
	{
		srb32xldflags_t *compData = (srb32xldflags_t *)malloc(numlinedefs * sizeof(srb32xldflags_t));
		for (int i = 0; i < numlinedefs; i++)
		{
			compData[i].flags = swap_endian16(linedefs[i].flags);
			compData[i].special = RemapLinedefSpecial(linedefs[i].special);

			if (linedefs[i].tag > 255)
			{
				compData[i].special = 0;
				compData[i].tag = 0;
				printf("Linedef %d has tag %d which is > 255.\n", i, linedefs[i].tag);
				linedefs[i].tag = 0;
			}
			else
				compData[i].tag = (uint8_t)linedefs[i].tag;
		}

		entry->SetData((byte *)compData, numlinedefs * sizeof(srb32xldflags_t));
		free(compData);
	}

	// SIDEDEFS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SIDEDEFS");
	entry->SetIsCompressed(true);

	sidetex_t sidetexes[256];
	int numsidetexes = 0;

	if (srb32xsegs && t1)
	{
		srb32xsidedef_t *compData = (srb32xsidedef_t *)malloc(numsidedefs * sizeof(srb32xsidedef_t));
		for (int i = 0; i < numsidedefs; i++)
		{
			compData[i].sector = sidedefs[i].sector;
			compData[i].rowoffset = sidedefs[i].rowoffset & 0xff;
			compData[i].textureoffset = (sidedefs[i].textureoffset & 0xfff) | ((sidedefs[i].rowoffset & 0x0f00) << 4);

			sidetex_t sidetex;
			sidetex.top = FindTexture(t1, sidedefs[i].toptexture);
			sidetex.mid = FindTexture(t1, sidedefs[i].midtexture);
			sidetex.bottom = FindTexture(t1, sidedefs[i].bottomtexture);

			compData[i].texIndex = FindSidetexIndex(sidetexes, &numsidetexes, sidetex);
		}

		entry->SetData((byte *)compData, numsidedefs * sizeof(srb32xsidedef_t));
		free(compData);
	}
	else
		entry->SetData((byte *)sidedefs, numsidedefs * sizeof(sidedef_t));

	if (srb32xsegs)
	{
		// SIDETEX
		entry = new WADEntry();
		Listable::Add(entry, (Listable **)&head);
		entry->SetName("SIDETEX");
		entry->SetIsCompressed(true);

		entry->SetData((byte*)sidetexes, sizeof(sidetex_t) * numsidetexes);
	}

	// VERTEXES
	if (srb32xsegs)
	{
		entry = new WADEntry();
		Listable::Add(entry, (Listable **)&head);
		entry->SetName("VERTEXES");
		entry->SetIsCompressed(loadFlags & LOADFLAGS_VERTEXES);
		vertex_t *temp = (vertex_t *)malloc(sizeof(vertex_t) * numvertexes);
		for (int32_t i = 0; i < numvertexes; i++)
		{
			temp[i].x = swap_endian16(vertexes[i].x);
			temp[i].y = swap_endian16(vertexes[i].y);
		}
		entry->SetData((byte *)temp, sizeof(vertex_t) * numvertexes);
		free(temp);
	}
	else
	{
		entry = new WADEntry();
		Listable::Add(entry, (Listable **)&head);
		entry->SetName("VERTEXES");
		entry->SetIsCompressed(false);
		jagVertex_t *temp = (jagVertex_t *)malloc(sizeof(jagVertex_t) * numvertexes);
		for (int32_t i = 0; i < numvertexes; i++)
		{
			temp[i].x = swap_endian32(vertexes[i].x << FRACBITS);
			temp[i].y = swap_endian32(vertexes[i].y << FRACBITS);
		}
		entry->SetData((byte *)temp, sizeof(jagVertex_t) * numvertexes);
		free(temp);
	}

	// SEGS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SEGS");
	if (srb32xsegs)
	{
		entry->SetIsCompressed(loadFlags & LOADFLAGS_SEGS);

		srb32xseg_t *newSegs = (srb32xseg_t *)malloc(sizeof(srb32xseg_t) * numsegs);
		seg_t *origSegs = segs;

		int numNotZero = 0;

		for (int32_t i = 0; i < numsegs; i++)
		{
			newSegs[i].v1 = swap_endian16(origSegs[i].v1);
			newSegs[i].v2 = swap_endian16(origSegs[i].v2);
			newSegs[i].sideoffset = origSegs[i].offset; // Swap it later... you'll see
			newSegs[i].linedef = swap_endian16(origSegs[i].linedef);

			int16_t side = origSegs[i].side;
			side &= 1;

			newSegs[i].sideoffset <<= 1;
			newSegs[i].sideoffset |= side;

			if (newSegs[i].sideoffset != 0)
				numNotZero++;

			newSegs[i].sideoffset = swap_endian16(newSegs[i].sideoffset);
		}

		entry->SetData((byte*)newSegs, numsegs * sizeof(srb32xseg_t));
		free(newSegs);
	}
	else
	{
		entry->SetIsCompressed(true);
		entry->SetData((byte *)segs, numsegs * sizeof(seg_t));
	}

	// SSECTORS
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SSECTORS");
	entry->SetIsCompressed(loadFlags & LOADFLAGS_SUBSECTORS);

	if (srb32xsegs)
	{
		srb32xsubsector_t *compData = (srb32xsubsector_t *)malloc(sizeof(srb32xsubsector_t) * (numsubsectors + 1));
		int i;
		for (i = 0; i < numsubsectors; i++)
		{
			compData[i].firstline = swap_endian16(subsectors[i].firstseg);
			compData[i].isector = swap_endian16(GetSectorFromSeg(subsectors[i].firstseg));
		}

		compData[i].firstline = swap_endian16(numsegs);
		compData[i].isector = swap_endian16(GetSectorFromSeg(0));

		entry->SetData((byte *)compData, (numsubsectors + 1) * sizeof(srb32xsubsector_t));
		free(compData);
	}
	else
	{
		entry->SetData((byte *)subsectors, numsubsectors * sizeof(subsector_t));
	}

	// NODES
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("NODES");
	entry->SetIsCompressed(loadFlags & LOADFLAGS_NODES);
	if (jagNodes)
		entry->SetData((byte *)jagNodes, numnodes * sizeof(jagnode_t));
	else if (srb32xsegs)
	{
/*		for (int i = 0; i < numnodes; i++)
		{
			nodes[i].dx = swap_endian16(nodes[i].dx);
			nodes[i].dy = swap_endian16(nodes[i].dy);
			nodes[i].x = swap_endian16(nodes[i].x);
			nodes[i].y = swap_endian16(nodes[i].y);
			nodes[i].children[0] = swap_endian16(nodes[i].children[0]);
			nodes[i].children[1] = swap_endian16(nodes[i].children[1]);
			
			for (int x = 0; x < 2; x++)
				for (int y = 0; y < 4; y++)
					nodes[i].bbox[x][y] = swap_endian16(nodes[i].bbox[x][y]);
		}*/
		srb32xnode_t *newNodes = CompressNodes(nodes, numnodes);
		entry->SetData((byte *)newNodes, numnodes * sizeof(srb32xnode_t));
	}
	else
	{
		// Convert from PC to Jaguar format
		jagnode_t *temp = (jagnode_t *)malloc(numnodes * sizeof(jagnode_t));
		for (int32_t i = 0; i < numnodes; i++)
		{
			temp[i].x = swap_endian32(nodes[i].x << FRACBITS);
			temp[i].y = swap_endian32(nodes[i].y << FRACBITS);
			temp[i].dx = swap_endian32(nodes[i].dx << FRACBITS);
			temp[i].dy = swap_endian32(nodes[i].dy << FRACBITS);
			
			for (int32_t j = 0; j < 2; j++)
			{
				for (int32_t k = 0; k < 4; k++)
					temp[i].bbox[j][k] = swap_endian32(nodes[i].bbox[j][k] << FRACBITS);
			}

			temp[i].children[0] = swap_endian32(nodes[i].children[0]);
			temp[i].children[1] = swap_endian32(nodes[i].children[1]);
		}
		entry->SetData((byte *)temp, numnodes * sizeof(jagnode_t));
		free(temp);
	}

#ifdef DYNAMIC_SECTORS
	// First, find out how many static sectors we have and how many dynamic sectors we have.
	int numDynamic = 0;
	int numStatic = 0;
	for (int i = 0; i < numsectors; i++)
	{
		if (IsDynamicSector(&fullSectors[i], hasLightning))
			numDynamic++;
		else
			numStatic++;
	}

	// SECTORS (uncompressed) - these are static and stay in ROM
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SECTORS");
	entry->SetIsCompressed(false);

	if (srb32xsegs)
	{
		staticsector_t *compData = (staticsector_t *)malloc(numStatic * sizeof(staticsector_t));

		int ci = 0;
		for (int i = 0; i < numsectors; i++)
		{
			if (IsDynamicSector(&fullSectors[i], hasLightning))
				continue;

			compData[ci] = fullSectors[i];
			compData[ci].ceilingheight = swap_endian32(compData[ci].ceilingheight);
			compData[ci].floorheight = swap_endian32(compData[ci].floorheight);
			compData[ci].floor_offs = swap_endian16(compData[ci].floor_offs);
			compData[ci].fofsec = swap_endian16(compData[ci].fofsec);
			compData[ci].heightsec = swap_endian16(compData[ci].heightsec);
			compData[ci].specialdata = swap_endian16(0);
			compData[ci].specline = swap_endian16(compData[ci].specline);
			compData[ci].extrasecdata = swap_endian16(compData[ci].extrasecdata);
			ci++;
		}

		entry->SetData((byte *)compData, numStatic * sizeof(staticsector_t));
		free(compData);
	}

	// DSECTORS (compressed) - these are dynamic and stay in RAM
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("DSECTORS");
	entry->SetIsCompressed(true);

	if (srb32xsegs)
	{
		srb32xsector_t *compData = (srb32xsector_t *)malloc(numDynamic * sizeof(srb32xsector_t));
		int ci = 0;
		for (int i = 0; i < numsectors; i++)
		{
			if (!IsDynamicSector(&fullSectors[i], hasLightning))
				continue;

			compData[ci].floorheight = swap_endian16(fullSectors[i].floorheight >> 16);
			compData[ci].ceilingheight = swap_endian16(fullSectors[i].ceilingheight >> 16);
			compData[ci].floorpic = fullSectors[i].floorpic;
			compData[ci].ceilingpic = fullSectors[i].ceilingpic;
			compData[ci].lightlevel = fullSectors[i].lightlevel;
			compData[ci].special = fullSectors[i].special;
			compData[ci].tag = fullSectors[i].tag;

			ci++;
		}

		entry->SetData((byte *)compData, numDynamic * sizeof(srb32xsector_t));
		free(compData);
	}
	else
		entry->SetData((byte *)sectors, numDynamic * sizeof(sector_t));
#else
	// SECTORS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SECTORS");
	entry->SetIsCompressed(true);

	if (srb32xsegs)
	{
		srb32xsector_t *compData = (srb32xsector_t *)malloc(numsectors * sizeof(srb32xsector_t));
		for (int i = 0; i < numsectors; i++)
		{
			compData[i].floorheight = sectors[i].floorheight;
			compData[i].ceilingheight = sectors[i].ceilingheight;
			compData[i].floorpic = FindFlat(fList, sectors[i].floorpic);
			compData[i].ceilingpic = FindFlat(fList, sectors[i].ceilingpic);
			compData[i].lightlevel = (uint8_t)sectors[i].lightlevel;
			compData[i].special = (uint8_t)sectors[i].special;

			if (sectors[i].tag > 255)
			{
				printf("Sector %d has tag %d, which is > 255.\n", i, sectors[i].tag);
				sectors[i].tag = 0;
				compData[i].tag = 0;
			}
			else
				compData[i].tag = (uint8_t)sectors[i].tag;
		}

		entry->SetData((byte *)compData, numsectors * sizeof(srb32xsector_t));
		free(compData);
	}
	else
		entry->SetData((byte *)sectors, numsectors * sizeof(sector_t));
#endif

	// REJECT
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("REJECT");
	entry->SetIsCompressed(loadFlags & LOADFLAGS_REJECT);

	if (this->rejectSize > 0)	
	{ // Vic's 50% compression of the REJECT table technique
		int i, j;
		uint8_t *out;
		uint8_t* data = this->reject;
		int outsize;
		unsigned outbit, outbyte;

		// compress the reject table, assuming it is symmetrical
		outsize = (numsectors + 1) * numsectors / 2;
		outsize = (outsize + 7) / 8;

		byte *rejectmatrix = (byte *)malloc(outsize);

		memset(rejectmatrix, 0, outsize);
		
		outbit = 1;
		outbyte = 0;
		out = rejectmatrix;

		for (i = 0; i < numsectors; i++) {
			unsigned k = i * numsectors;
			unsigned bit = 1 << ((k + i) & 7);
			for (j = i; j < numsectors; j++) {
				unsigned bytenum = (k + j) / 8;

				if (data[bytenum] & bit)
				{
					out[outbyte] |= outbit;
				}

				bit <<= 1;
				if (bit > 0xff) {
					bit = 1;
				}

				outbit <<= 1;
				if (outbit > 0xff) {
					outbyte++;
					outbit = 1;
				}
			}
		}

		entry->SetData(rejectmatrix, outsize);
		free(rejectmatrix);
	}
	else
	{
		byte empty[1];
		entry->SetIsCompressed(false);
		entry->SetData(empty, 0);
	}

	// BLOCKMAP
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("BLOCKMAP");
	entry->SetIsCompressed(loadFlags & LOADFLAGS_BLOCKMAP);

	{
		// Gotta byteswap it all
		uint16_t *blockmapPtr = (uint16_t *)blockmap;
		uint16_t *temp = (uint16_t *)malloc(blockmapSize);
		for (int32_t i = 0; i < blockmapSize / 2; i++)
			temp[i] = swap_endian16(blockmapPtr[i]);

		entry->SetData((byte*)temp, blockmapSize);
		free(temp);
	}

	return head;
}

WADEntry *WADMap::CreatePC(const char *mapname)
{
	WADEntry *head = NULL;

	// Header
	WADEntry *entry = new WADEntry();
	entry->SetName(mapname);
	Listable::Add(entry, (Listable **)&head);

	// THINGS
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("THINGS");
	entry->SetIsCompressed(false);
	entry->SetData((byte *)things, numthings * sizeof(mapthing_t));

	// LINEDEFS
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("LINEDEFS");
	entry->SetIsCompressed(false);
	entry->SetData((byte *)linedefs, numlinedefs * sizeof(linedef_t));

	// SIDEDEFS
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SIDEDEFS");
	entry->SetIsCompressed(false);
	entry->SetData((byte *)sidedefs, numsidedefs * sizeof(sidedef_t));

	// VERTEXES
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("VERTEXES");
	entry->SetIsCompressed(false);
	entry->SetData((byte *)vertexes, numvertexes * sizeof(vertex_t));

	// SEGS
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SEGS");
	entry->SetIsCompressed(false);
	entry->SetData((byte *)segs, numsegs * sizeof(seg_t));

	// SSECTORS
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SSECTORS");
	entry->SetIsCompressed(false);
	entry->SetData((byte *)subsectors, numsubsectors * sizeof(subsector_t));

	// NODES
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("NODES");
	entry->SetIsCompressed(false);
	if (jagNodes)
	{
		// Convert from Jaguar to PC format
		node_t *temp = (node_t *)malloc(numnodes * sizeof(node_t));
		for (int32_t i = 0; i < numnodes; i++)
		{
			temp[i].x = (int16_t)(swap_endian32(jagNodes[i].x) >> FRACBITS);
			temp[i].y = (int16_t)(swap_endian32(jagNodes[i].y) >> FRACBITS);
			temp[i].dx = (int16_t)(swap_endian32(jagNodes[i].dx) >> FRACBITS);
			temp[i].dy = (int16_t)(swap_endian32(jagNodes[i].dy) >> FRACBITS);

			for (int32_t j = 0; j < 2; j++)
			{
				for (int32_t k = 0; k < 4; k++)
					temp[i].bbox[j][k] = (int16_t)(swap_endian32(jagNodes[i].bbox[j][k]) >> FRACBITS);
			}

			temp[i].children[0] = (uint16_t)(swap_endian32(jagNodes[i].children[0]));
			temp[i].children[1] = (uint16_t)(swap_endian32(jagNodes[i].children[1]));
		}
		entry->SetData((byte *)temp, numnodes * sizeof(node_t));
		free(temp);
	}
	else
		entry->SetData((byte *)nodes, numnodes * sizeof(node_t));

	// SECTORS
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SECTORS");
	entry->SetIsCompressed(false);
	entry->SetData((byte *)sectors, numsectors * sizeof(sector_t));

	// REJECT
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("REJECT");
	entry->SetIsCompressed(false);
	entry->SetData(reject, rejectSize);

	// BLOCKMAP
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("BLOCKMAP");
	entry->SetIsCompressed(false);
	entry->SetData(blockmap, blockmapSize);

	return head;
}

WADMap::WADMap(WADEntry *head)
{
	things = NULL;
	linedefs = NULL;
	sidedefs = NULL;
	vertexes = NULL;
	segs = NULL;
	subsectors = NULL;
	nodes = NULL;
	jagNodes = NULL;
	sectors = NULL;
	reject = NULL;
	blockmap = NULL;

	numthings = 0;
	numlinedefs = 0;
	numsidedefs = 0;
	numvertexes = 0;
	numsegs = 0;
	numsubsectors = 0;
	numnodes = 0;
	numsectors = 0;
	rejectSize = 0;
	blockmapSize = 0;

	name = strdup(head->GetName());

	WADEntry *entry;

	bool isJaguar = false;
	
	// THINGS
	entry = (WADEntry *)head->next;
	if (entry->IsCompressed())
	{
		isJaguar = true;
		things = (mapthing_t *)entry->Decompress();
	}
	else
		things = (mapthing_t*)memdup(entry->GetData(), entry->GetDataLength());

	numthings = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(mapthing_t));

	// LINEDEFS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
	{
		isJaguar = true;
		linedefs = (linedef_t *)entry->Decompress();
	}
	else
		linedefs = (linedef_t *)memdup(entry->GetData(), entry->GetDataLength());

	numlinedefs = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(linedef_t));

	// SIDEDEFS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		sidedefs = (sidedef_t *)entry->Decompress();
	else
		sidedefs = (sidedef_t *)memdup(entry->GetData(), entry->GetDataLength());

	numsidedefs = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(sidedef_t));

	// VERTEXES
	entry = (WADEntry *)entry->next;
	if (isJaguar)
	{
		jagVertex_t *temp;
		if (entry->IsCompressed())
			temp = (jagVertex_t *)entry->Decompress();
		else
			temp = (jagVertex_t *)memdup(entry->GetData(), entry->GetDataLength());

		int16_t numjagVertexes = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(jagVertex_t));
		vertexes = (vertex_t *)malloc(numjagVertexes * sizeof(vertex_t));

		for (int i = 0; i < numjagVertexes; i++)
		{
			vertexes[i].x = (int16_t)(swap_endian32(temp[i].x) >> FRACBITS); // And nothing of value was lost...
			vertexes[i].y = (int16_t)(swap_endian32(temp[i].y) >> FRACBITS);
		}
		free(temp);
	}
	else
	{
		if (entry->IsCompressed())
			vertexes = (vertex_t *)entry->Decompress();
		else
			vertexes = (vertex_t *)memdup(entry->GetData(), entry->GetDataLength());

		numvertexes = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(vertex_t));
	}

	// SEGS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		segs = (seg_t *)entry->Decompress();
	else
		segs = (seg_t *)memdup(entry->GetData(), entry->GetDataLength());

	numsegs = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(seg_t));

	// SSECTORS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		subsectors = (subsector_t *)entry->Decompress();
	else
		subsectors = (subsector_t *)memdup(entry->GetData(), entry->GetDataLength());

	numsubsectors = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(subsector_t));

	// NODES
	entry = (WADEntry *)entry->next;
	if (isJaguar)
	{
		// These are byte swapped, but we won't bother since
		// we won't be manually editing nodes.
		if (entry->IsCompressed())
			jagNodes = (jagnode_t *)entry->Decompress();
		else
			jagNodes = (jagnode_t *)memdup(entry->GetData(), entry->GetDataLength());

		numnodes = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(jagnode_t));
	}
	else
	{
		if (entry->IsCompressed())
			nodes = (node_t *)entry->Decompress();
		else
			nodes = (node_t *)memdup(entry->GetData(), entry->GetDataLength());

		numnodes = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(node_t));
	}

	// SECTORS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		sectors = (sector_t *)entry->Decompress();
	else
		sectors = (sector_t *)memdup(entry->GetData(), entry->GetDataLength());

	numsectors = (int16_t)(entry->GetUnCompressedDataLength() / sizeof(sector_t));

	// REJECT
	entry = (WADEntry *)entry->next;
	if (entry && !strcmp(entry->GetName(), "REJECT"))
	{
		if (entry->IsCompressed())
			reject = (byte *)entry->Decompress();
		else
			reject = (byte *)memdup(entry->GetData(), entry->GetDataLength());
		rejectSize = entry->GetUnCompressedDataLength();
	}
	else
	{
		reject = (byte *)malloc(1);
		rejectSize = 0;
	}

	if (!entry)
		return;

	// BLOCKMAP
	entry = (WADEntry *)entry->next;
	if (entry && !strcmp(entry->GetName(), "BLOCKMAP"))
	{
		blockmapSize = entry->GetUnCompressedDataLength();

		if (isJaguar)
		{
			// Need to byte swap each entry
			uint16_t *temp;
			if (entry->IsCompressed())
				temp = (uint16_t *)entry->Decompress();
			else
				temp = (uint16_t *)memdup(entry->GetData(), entry->GetDataLength());

			for (int i = 0; i < blockmapSize / 2; i++)
				temp[i] = swap_endian16(temp[i]);

			blockmap = (byte *)temp;
		}
		else
		{
			if (entry->IsCompressed())
				blockmap = (byte *)entry->Decompress();
			else
				blockmap = (byte *)memdup(entry->GetData(), entry->GetDataLength());
		}
	}
	else
	{
		blockmap = (byte *)malloc(1);
		blockmapSize = 0;
	}
}

WADMap::~WADMap()
{
	if (name)
		free(name);

	if (things)
		free(things);

	if (linedefs)
		free(linedefs);

	if (sidedefs)
		free(sidedefs);

	if (vertexes)
		free(vertexes);

	if (segs)
		free(segs);

	if (subsectors)
		free(subsectors);

	if (nodes)
		free(nodes);

	if (jagNodes)
		free(jagNodes);

	if (sectors)
		free(sectors);

	if (reject)
		free(reject);

	if (blockmap)
		free(blockmap);
}
