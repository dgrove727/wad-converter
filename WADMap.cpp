#include "WADMap.h"
#include <stdlib.h>
#include <stdio.h>

WADEntry *WADMap::CreateJaguar(const char *mapname)
{
	WADEntry *head = NULL;

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
	entry->SetIsCompressed(true);
	entry->SetData((byte *)segs, numsegs * sizeof(seg_t));

	// SSECTORS (compressed)
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
	entry->SetData(blockmap, blockmapSize);

	return head;
}

WADEntry *WADMap::CreatePC(const char *mapname)
{
	printf("Not supported yet.\n");
	return NULL;
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
			jagNodes = (node_t *)entry->Decompress();
		else
			jagNodes = (node_t *)memdup(entry->GetData(), entry->GetDataLength());

		numnodes = entry->GetUnCompressedDataLength() / sizeof(node_t);		
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
	if (entry->IsCompressed())
		reject = (byte *)entry->Decompress();
	else
		reject = (byte *)memdup(entry->GetData(), entry->GetDataLength());
	rejectSize = entry->GetUnCompressedDataLength();

	// BLOCKMAP
	entry = (WADEntry *)entry->next;
	if (entry->IsCompressed())
		blockmap = (byte *)entry->Decompress();
	else
		blockmap = (byte *)memdup(entry->GetData(), entry->GetDataLength());
	blockmapSize = entry->GetUnCompressedDataLength();
}

WADMap::~WADMap()
{
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
