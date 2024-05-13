#pragma once
#include "Exporter.h"
#include <stdio.h>

struct Exporter_32X : Exporter
{
protected:
	FILE *f;

public:
	virtual void Execute();

	Exporter_32X(WADEntry *entries, FILE *f);
	virtual ~Exporter_32X();
};