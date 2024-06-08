#pragma once
#include "Exporter.h"
#include <stdio.h>

struct Exporter_PC : Exporter
{
protected:
	FILE *f;

public:
	virtual void Execute();

	Exporter_PC(WADEntry *entries, FILE *f);
	virtual ~Exporter_PC();
};