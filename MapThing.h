#pragma once
#include "Listable.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "WADMap.h"

struct MapThing : Listable
{
	mapthing_t thing;

	MapThing()
	{
	}

	virtual ~MapThing()
	{
	}
};
