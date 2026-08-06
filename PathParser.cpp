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

    outputPath = new BezierPath();
    NSVGimage *nImage = (NSVGimage *)image;

    // Iterate the points in the path
    for (NSVGshape *shape = nImage->shapes; shape; shape = shape->next) {
        for (NSVGpath *path = shape->paths; path; path = path->next) {
            float *p = path->pts;
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
        }
    }
}

byte *PathParser::CreateLump(size_t *lumpSize)
{
    if (!outputPath)
        return NULL;

    size_t count = Listable::GetCount(outputPath->segments);
    *lumpSize = sizeof(int16_t) * 8 * count;

    byte *lump = (byte *)malloc(*lumpSize);
    int16_t *cursor = (int16_t*)lump;

    BezierPoint *node;
    for (node = outputPath->segments; node; node = (BezierPoint *)node->next)
    {
        *cursor++ = node->x0;
        *cursor++ = node->y0;
        *cursor++ = node->x1;
        *cursor++ = node->y1;
        *cursor++ = node->x2;
        *cursor++ = node->y2;
        *cursor++ = node->x3;
        *cursor++ = node->y3;
    }

    return lump;
}

PathParser::~PathParser()
{
    if (image)
        nsvgDelete((NSVGimage *)image);
}
