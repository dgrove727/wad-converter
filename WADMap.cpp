#include "WADMap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
	short v1, v2;
	short sideoffset;
	short linedef;
} srb32xseg_t;

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
	int numNewSidedefs = 0;

	for (int i = 0; i < numsidedefs; i++)
	{
		if (sidedefs[i].sector == -1)
			continue;

		linedef_t *ld = this->linedefs;
		for (int j = numlinedefs; j--; ld++)
		{
			if (ld->sidenum[0] == i)
				ld->sidenum[0] = numNewSidedefs;

			if (ld->sidenum[1] == i)
				ld->sidenum[1] = numNewSidedefs;
		}

		for (int j = i + 1; j < numsidedefs; j++)
		{
			if (sidedefs[j].sector == -1)
				continue;

			if (IdenticalSidedefs(&sidedefs[i], &sidedefs[j]))
			{
				// Find the linedefs that belong to this one
				ld = linedefs;
				for (int k = numlinedefs; k--; ld++)
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

	sidedef_t *newSidedefs = (sidedef_t *)malloc(numNewSidedefs * sizeof(sidedef_t));

	// Copy the sidedefs to their new block of memory.
	int z = 0;
	for (int i = 0; i < numsidedefs; i++)
	{
		if (sidedefs[i].sector == -1)
			continue;

		memcpy(&newSidedefs[z++], &sidedefs[i], sizeof(sidedef_t));
	}

	free(sidedefs);
	sidedefs = newSidedefs;
	numsidedefs = numNewSidedefs;
}

WADEntry *WADMap::CreateJaguar(const char *mapname, bool srb32xsegs)
{
	WADEntry *head = NULL;

	CompressSidedefs();

	// Header
	WADEntry *entry = new WADEntry();
	entry->SetName(mapname);
	Listable::Add(entry, (Listable **)&head);

	// THINGS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("THINGS");
	entry->SetIsCompressed(true);
	entry->SetData((byte*)things, numthings * sizeof(mapthing_t));

	// LINEDEFS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("LINEDEFS");
	entry->SetIsCompressed(true);
	entry->SetData((byte *)linedefs, numlinedefs * sizeof(linedef_t));

	// SIDEDEFS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SIDEDEFS");
	entry->SetIsCompressed(true);
	entry->SetData((byte *)sidedefs, numsidedefs * sizeof(sidedef_t));

	// VERTEXES
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("VERTEXES");
	entry->SetIsCompressed(false);
	jagVertex_t *temp = (jagVertex_t *)malloc(sizeof(jagVertex_t) * numvertexes);
	for (int i = 0; i < numvertexes; i++)
	{
		temp[i].x = swap_endian32(vertexes[i].x << FRACBITS);
		temp[i].y = swap_endian32(vertexes[i].y << FRACBITS);
	}
	entry->SetData((byte*)temp, sizeof(jagVertex_t) * numvertexes);
	free(temp);

	// SEGS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SEGS");
	if (srb32xsegs)
	{
		entry->SetIsCompressed(false);

		srb32xseg_t *newSegs = (srb32xseg_t *)malloc(sizeof(srb32xseg_t) * numsegs);
		seg_t *origSegs = segs;

		for (int i = 0; i < numsegs; i++)
		{
			newSegs[i].v1 = swap_endian16(origSegs[i].v1);
			newSegs[i].v2 = swap_endian16(origSegs[i].v2);
			newSegs[i].sideoffset = origSegs[i].offset; // Swap it later... you'll see
			newSegs[i].linedef = swap_endian16(origSegs[i].linedef);

			int16_t side = origSegs[i].side;
			side &= 1;

			newSegs[i].sideoffset <<= 1;
			newSegs[i].sideoffset |= side;

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
	entry->SetIsCompressed(false);
	entry->SetData((byte *)subsectors, numsubsectors * sizeof(subsector_t));

	// NODES
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("NODES");
	entry->SetIsCompressed(false);
	if (jagNodes)
		entry->SetData((byte *)jagNodes, numnodes * sizeof(jagnode_t));
	else
	{
		// Convert from PC to Jaguar format
		jagnode_t *temp = (jagnode_t *)malloc(numnodes * sizeof(jagnode_t));
		for (int i = 0; i < numnodes; i++)
		{
			temp[i].x = swap_endian32(nodes[i].x << FRACBITS);
			temp[i].y = swap_endian32(nodes[i].y << FRACBITS);
			temp[i].dx = swap_endian32(nodes[i].dx << FRACBITS);
			temp[i].dy = swap_endian32(nodes[i].dy << FRACBITS);
			
			for (int j = 0; j < 2; j++)
			{
				for (int k = 0; k < 4; k++)
					temp[i].bbox[j][k] = swap_endian32(nodes[i].bbox[j][k] << FRACBITS);
			}

			temp[i].children[0] = swap_endian32(nodes[i].children[0]);
			temp[i].children[1] = swap_endian32(nodes[i].children[1]);
		}
		entry->SetData((byte *)temp, numnodes * sizeof(jagnode_t));
		free(temp);
	}

	// SECTORS (compressed)
	entry = new WADEntry();
	Listable::Add(entry, (Listable **)&head);
	entry->SetName("SECTORS");
	entry->SetIsCompressed(true);
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

	{
		// Gotta byteswap it all
		unsigned short *blockmapPtr = (unsigned short *)blockmap;
		unsigned short *temp = (unsigned short *)malloc(blockmapSize);
		for (int i = 0; i < blockmapSize / 2; i++)
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
		for (int i = 0; i < numnodes; i++)
		{
			temp[i].x = (int16_t)(swap_endian32(jagNodes[i].x) >> FRACBITS);
			temp[i].y = (int16_t)(swap_endian32(jagNodes[i].y) >> FRACBITS);
			temp[i].dx = (int16_t)(swap_endian32(jagNodes[i].dx) >> FRACBITS);
			temp[i].dy = (int16_t)(swap_endian32(jagNodes[i].dy) >> FRACBITS);

			for (int j = 0; j < 2; j++)
			{
				for (int k = 0; k < 4; k++)
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

	numthings = entry->GetUnCompressedDataLength() / sizeof(mapthing_t);

	// LINEDEFS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
	{
		isJaguar = true;
		linedefs = (linedef_t *)entry->Decompress();
	}
	else
		linedefs = (linedef_t *)memdup(entry->GetData(), entry->GetDataLength());

	numlinedefs = entry->GetUnCompressedDataLength() / sizeof(linedef_t);

	// SIDEDEFS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		sidedefs = (sidedef_t *)entry->Decompress();
	else
		sidedefs = (sidedef_t *)memdup(entry->GetData(), entry->GetDataLength());

	numsidedefs = entry->GetUnCompressedDataLength() / sizeof(sidedef_t);

	// VERTEXES
	entry = (WADEntry *)entry->next;
	if (isJaguar)
	{
		jagVertex_t *temp;
		if (entry->IsCompressed())
			temp = (jagVertex_t *)entry->Decompress();
		else
			temp = (jagVertex_t *)memdup(entry->GetData(), entry->GetDataLength());

		int16_t numjagVertexes = entry->GetUnCompressedDataLength() / sizeof(jagVertex_t);
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

		numvertexes = entry->GetUnCompressedDataLength() / sizeof(vertex_t);
	}

	// SEGS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		segs = (seg_t *)entry->Decompress();
	else
		segs = (seg_t *)memdup(entry->GetData(), entry->GetDataLength());

	numsegs = entry->GetUnCompressedDataLength() / sizeof(seg_t);

	// SSECTORS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		subsectors = (subsector_t *)entry->Decompress();
	else
		subsectors = (subsector_t *)memdup(entry->GetData(), entry->GetDataLength());

	numsubsectors = entry->GetUnCompressedDataLength() / sizeof(subsector_t);

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

		numnodes = entry->GetUnCompressedDataLength() / sizeof(jagnode_t);		
	}
	else
	{
		if (entry->IsCompressed())
			nodes = (node_t *)entry->Decompress();
		else
			nodes = (node_t *)memdup(entry->GetData(), entry->GetDataLength());

		numnodes = entry->GetUnCompressedDataLength() / sizeof(node_t);
	}

	// SECTORS
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		sectors = (sector_t *)entry->Decompress();
	else
		sectors = (sector_t *)memdup(entry->GetData(), entry->GetDataLength());

	numsectors = entry->GetUnCompressedDataLength() / sizeof(sector_t);

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
			unsigned short *temp;
			if (entry->IsCompressed())
				temp = (unsigned short *)entry->Decompress();
			else
				temp = (unsigned short *)memdup(entry->GetData(), entry->GetDataLength());

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
