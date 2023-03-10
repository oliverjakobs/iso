#include "iso.h"

#include <ignis/renderer/renderer.h>

vec2 isoToCartesian(vec2 iso)
{
    vec2 point = {
        (2 * iso.y + iso.x) * .5f,
        (2 * iso.y - iso.x) * .5f
    };
    return point;
}

vec2 cartesianToIso(vec2 cartesian)
{
    vec2 iso = {
        cartesian.x - cartesian.y,
        (cartesian.x + cartesian.y) * .5f
    };
    return iso;
}

void isoMapInit(IsoMap* map, uint32_t* grid, uint32_t width, uint32_t height, float tile_size, float tile_offset)
{
    map->grid = grid;
    map->width = width;
    map->height = height;
    map->tile_size = tile_size;
    map->tile_offset = tile_offset;
    map->origin = vec2_zero();
}

void isoMapSetOrigin(IsoMap* map, vec2 origin)
{
    map->origin = origin;
}

vec2 screenToWorld(const IsoMap* map, vec2 point)
{
    return isoToCartesian(vec2_sub(point, map->origin));
}

vec2 worldToScreen(const IsoMap* map, vec2 point)
{
    return vec2_add(cartesianToIso(point), map->origin);
}

uint32_t tileClip(const IsoMap* map, float f) { return (uint32_t)floorf(f / map->tile_size); }

vec2 getTileScreenPos(const IsoMap* map, uint32_t col, uint32_t row)
{
    vec2 point = { col - .5f, row + .5f };
    return worldToScreen(map, vec2_mult(point, map->tile_size));
}

vec2 getTileScreenCenter(const IsoMap* map, uint32_t col, uint32_t row)
{
    vec2 point = { col + .5f, row + .5f };
    return worldToScreen(map, vec2_mult(point, map->tile_size));
}

void renderMap(const IsoMap* map, const IgnisTexture2D* texture_atlas)
{
    for (uint32_t i = 0; i < map->width * map->height; i++)
    {
        uint32_t col = i % map->width;
        uint32_t row = i / map->width;

        vec2 pos = getTileScreenPos(map, col, row);
        IgnisRect rect = {
            pos.x, pos.y,
            map->tile_size * 2.0f,
            map->tile_size + map->tile_offset
        };
        uint32_t frame = map->grid[i];
        ignisBatch2DRenderTextureFrame(texture_atlas, rect, frame);
    }
}

void highlightTile(const IsoMap* map, vec2 world)
{
    uint32_t col = tileClip(map, world.x);
    uint32_t row = tileClip(map, world.y);

    if (col >= map->width || row >= map->height)
        return;

    // hightlight isometric version / screen
    vec2 center = getTileScreenCenter(map, col, row);
    ignisPrimitives2DRenderRhombus(center.x, center.y, 2 * map->tile_size, map->tile_size, IGNIS_WHITE);
    ignisPrimitives2DFillCircle(center.x, center.y, 2, IGNIS_WHITE);

    // hightlight cartesian version / world
    float tile_size = map->tile_size;
    ignisPrimitives2DRenderRect(col * tile_size, row * tile_size, tile_size, tile_size, IGNIS_WHITE);
    ignisPrimitives2DFillCircle(world.x, world.y, 3, IGNIS_BLUE);
}