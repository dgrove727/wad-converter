#pragma once
#include "Exporter.h"
#include <stdio.h>

struct Exporter_Jaguar : Exporter
{
protected:
	FILE *f;

public:
	virtual void Execute();

	Exporter_Jaguar(WADEntry *entries, FILE *f);
	virtual ~Exporter_Jaguar();
};