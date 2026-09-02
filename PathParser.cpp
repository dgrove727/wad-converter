#include "PathParser.h"
#include <math.h>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

static float scale = 1.0f; // 1 SVG unit = 1 map unit
static float offsetX = 0.0f;
static float offsetY = 0.0f;
static int   flipY = 1; // SVG Y grows downward, Doom upward

int16_t PathParser::toDoom(float v, bool is_y)
{
    float t = v * scale;

    NSVGimage *nImage = (NSVGimage *)image;

    if (is_y)
        t = nImage->height - t;                 /* or (height - t) if you prefer */

    t += is_y ? offsetY : offsetX;

    /* clamp to signed 16-bit */
    if (t > 32767.0f)
        t = 32767.0f;

    if (t < -32768.0f)
        t = -32768.0f;

    return (int16_t)lroundf(t);
}

void PathParser::ParsePath(const char *svgFile)
{
    if (image)
        nsvgDelete((NSVGimage *)image);

    image = nsvgParseFromFile(svgFile, "px", 96.0f);

    if (!image)
        return;

    NSVGimage *nImage = (NSVGimage *)image;

    // Iterate the points in the path
    for (NSVGshape *shape = nImage->shapes; shape; shape = shape->next) {
        for (NSVGpath *path = shape->paths; path; path = path->next) {
            float *p = path->pts;

            BezierPath *outputPath = new BezierPath();
            outputPath->id = (int16_t)atoi(shape->id);
            
            for (int i = 0; i < path->npts - 1; i += 3) {
                // p[0],p[1] = start
                // p[2],p[3] = control 1
                // p[4],p[5] = control 2
                // p[6],p[7] = end

                BezierPoint *bp = new BezierPoint();
                Listable::Add(bp, (Listable **)&outputPath->segments);

                bp->x0 = toDoom(p[0], false);
                bp->y0 = toDoom(p[1], true);
                bp->x1 = toDoom(p[2], false);
                bp->y1 = toDoom(p[3], true);
                bp->x2 = toDoom(p[4], false);
                bp->y2 = toDoom(p[5], true);
                bp->x3 = toDoom(p[6], false);
                bp->y3 = toDoom(p[7], true);

                p += 6;   // advance to next cubic (the end point becomes the next start)
            }

            Listable::Add(outputPath, (Listable **)&outputPaths);
        }
    }
}

typedef struct
{
    int16_t id;
    int16_t numSegments;
    int16_t startAddr;
} bezier_header_t;

uint8_t *PathParser::CreateLump(size_t *lumpSize)
{
    if (!outputPaths)
        return NULL;

    size_t pointCount = 0;

    BezierPath *pathNode;
    for (pathNode = outputPaths; pathNode; pathNode = (BezierPath *)pathNode->next)
        pointCount += Listable::GetCount(pathNode->segments);

    size_t headerSize = Listable::GetCount(outputPaths) * sizeof(bezier_header_t); // One for ID, another for Start Address

    *lumpSize = sizeof(int16_t); // # of paths
    *lumpSize += headerSize;
    *lumpSize += sizeof(int16_t) * pointCount * 8;

    uint8_t *lump = (uint8_t *)malloc(*lumpSize);
    int16_t *cursor = (int16_t*)lump;

    // Write out header:
    *cursor++ = swap_endian16((int16_t)Listable::GetCount(outputPaths)); // # paths

    for (pathNode = outputPaths; pathNode; pathNode = (BezierPath *)pathNode->next)
    {
        cursor++;
        cursor++;
        cursor++;
    }

    for (pathNode = outputPaths; pathNode; pathNode = (BezierPath *)pathNode->next)
    {
        pathNode->writingAddress = (uint8_t*)cursor;

        BezierPoint *node;
        for (node = pathNode->segments; node; node = (BezierPoint *)node->next)
        {
            *cursor++ = swap_endian16(node->x0);
            *cursor++ = swap_endian16(node->y0);
            *cursor++ = swap_endian16(node->x1);
            *cursor++ = swap_endian16(node->y1);
            *cursor++ = swap_endian16(node->x2);
            *cursor++ = swap_endian16(node->y2);
            *cursor++ = swap_endian16(node->x3);
            *cursor++ = swap_endian16(node->y3);
        }
    }

    // Write out path addresses
    cursor = (int16_t*)lump;
    cursor++;
    for (pathNode = outputPaths; pathNode; pathNode = (BezierPath *)pathNode->next)
    {
        *cursor++ = swap_endian16(pathNode->id);
        *cursor++ = swap_endian16((int16_t)Listable::GetCount(pathNode->segments));
        int16_t addr = pathNode->writingAddress - (uint8_t *)lump;
        *cursor++ = swap_endian16(addr);
    }

    return lump;
}

PathParser::~PathParser()
{
    if (image)
        nsvgDelete((NSVGimage *)image);
}
