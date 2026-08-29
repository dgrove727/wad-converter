#pragma once
#include <stdint.h>
#include "Listable.h"
#include "common.h"

struct BezierPoint : Listable
{
	int16_t x0;
	int16_t y0;
	int16_t x1;
	int16_t y1;
	int16_t x2;
	int16_t y2;
	int16_t x3;
	int16_t y3;
};

struct BezierPath : Listable
{
	BezierPoint *segments;
	int16_t id;
	uint8_t *writingAddress; // Only used during saving

	BezierPath() {}
	virtual ~BezierPath()
	{
		Listable::RemoveAll((Listable**)&segments);
	}
};

struct PathParser : Listable
{
protected:
	void *image;

public:
	float scale;
	float offsetX;
	float offsetY;

	BezierPath *outputPaths;

	int16_t toDoom(float v, bool is_y);
	void ParsePath(const char *svgFile);
	byte *CreateLump(size_t *lumpSize);

	PathParser()
	{
		scale = 1.0f;
		offsetX = 0.0f;
		offsetY = 0.0f;
		image = NULL;
		outputPaths = NULL;
	}

	virtual ~PathParser();
};
