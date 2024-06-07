#pragma once
#include <stdio.h>
#include "WADEntry.h"

struct Importer_Jaguar
{
protected:
	FILE *in_file;

public:
	virtual WADEntry *Execute();

	Importer_Jaguar(FILE *f);
	virtual ~Importer_Jaguar();
};