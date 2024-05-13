#pragma once
#include "WADEntry.h"

struct Exporter
{
protected:
	WADEntry *entries;

public:
	virtual void Execute() = 0;

	Exporter();
	virtual ~Exporter();
};