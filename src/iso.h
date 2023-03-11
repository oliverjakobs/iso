#ifndef ISO_H
#define ISO_H

#include <Ignis/Ignis.h>

#include "math/math.h"

vec2 isoToCartesian(vec2 iso);
vec2 cartesianToIso(vec2 cartesian);

typedef struct
{
    vec2 origin;

    uint32_t* grid;
    uint32_t width;
    uint32_t height;

    float tile_size;
} IsoMap;

void isoMapInit(IsoMap* map, uint32_t* grid, uint32_t width, uint32_t height, float tile_size);

void isoMapSetOrigin(IsoMap* map, vec2 origin);

vec2 screenToWorld(const IsoMap* map, vec2 point);
vec2 worldToScreen(const IsoMap* map, vec2 point);

void renderMap(const IsoMap* map, const IgnisTexture2D* texture_atlas);
void highlightTile(const IsoMap* map, vec2 world);

#endif // !ISO_H
