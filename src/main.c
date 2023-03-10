#include <Ignis/Ignis.h>

#include "Minimal/Application.h"
#include "Graphics/Renderer.h"
#include "math/math.h"

static void IgnisErrorCallback(ignisErrorLevel level, const char* desc)
{
    switch (level)
    {
    case IGNIS_WARN:     MINIMAL_WARN("%s", desc); break;
    case IGNIS_ERROR:    MINIMAL_ERROR("%s", desc); break;
    case IGNIS_CRITICAL: MINIMAL_CRITICAL("%s", desc); break;
    }
}

int show_info = 0;

float width, height;
mat4 screen_projection;

IgnisFont font;

typedef struct
{
    vec2 origin;

    uint32_t width;
    uint32_t height;

    float tile_size;
} IsoMap;

typedef struct
{
    vec2 position;
} Player;

int grid[] = {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
    3, 1, 1, 2, 1, 1, 1, 1, 1, 3,
    3, 1, 1, 2, 1, 1, 1, 1, 1, 3,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
    3, 1, 1, 1, 1, 1, 1, 2, 1, 3,
    3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};


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
        float width = (float)texture_atlas->width;
        float height = (float)texture_atlas->height;
        Batch2DRenderTextureFrame(texture_atlas, pos.x, pos.y, width, height, grid[i]);
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
    Primitives2DRenderRhombus(center.x, center.y, 2 * map->tile_size, map->tile_size, IGNIS_WHITE);
    Primitives2DFillCircle(center.x, center.y, 2, IGNIS_WHITE);

    // hightlight cartesian version / world
    float tile_size = map->tile_size;
    Primitives2DRenderRect(col * tile_size, row * tile_size, tile_size, tile_size, IGNIS_WHITE);
    Primitives2DFillCircle(world.x, world.y, 3, IGNIS_BLUE);
}

void renderPlayer(const IsoMap* map, const Player* player)
{
    vec2 screen = worldToScreen(map, player->position);
    Primitives2DFillCircle(screen.x, screen.y, 3, IGNIS_BLACK);
}

static void SetViewport(float w, float h)
{
    width = w;
    height = h;
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);

    Renderer2DSetViewProjection(screen_projection.v);
    Primitives2DSetViewProjection(screen_projection.v);
    FontRendererSetProjection(screen_projection.v);
    Batch2DSetViewProjection(screen_projection.v);
}

IsoMap map;
IgnisTexture2D tile_texture_atlas;

Player player;

int OnLoad(MinimalApp* app, uint32_t w, uint32_t h)
{
    /* ingis initialization */
    // ignisSetAllocator(FrostMemoryGetAllocator(), tb_mem_malloc, tb_mem_realloc, tb_mem_free);
    ignisSetErrorCallback(IgnisErrorCallback);

#ifdef _DEBUG
    int debug = 1;
#else
    int debug = 0;
#endif

    if (!ignisInit(debug))
    {
        MINIMAL_ERROR("[IGNIS] Failed to initialize Ignis");
        return MINIMAL_FAIL;
    }

    ignisEnableBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    /* renderer */
    Primitives2DInit();
    FontRendererInit();
    Renderer2DInit();
    Batch2DInit("res/shaders/batch.vert", "res/shaders/batch.frag");

    SetViewport((float)w, (float)h);

    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    FontRendererBindFontColor(&font, IGNIS_WHITE);

    MINIMAL_INFO("[GLFW] Version:        %s", glfwGetVersionString());
    MINIMAL_INFO("[OpenGL] Version:      %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL] Vendor:       %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL] Renderer:     %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL] GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis] Version:       %s", ignisGetVersionString());

    ignisCreateTexture2D(&tile_texture_atlas, "res/tiles.png", 1, 4, 0, NULL);

    map.origin = (vec2){ width * 0.5f, 100.0f };
    map.width = 10;
    map.height = 10;
    map.tile_size = 50.0f;

    player.position = (vec2){ 5 * map.tile_size, 5 * map.tile_size };

    return MINIMAL_OK;
}

void OnDestroy(MinimalApp* app)
{
    ignisDeleteFont(&font);

    Batch2DDestroy();
    FontRendererDestroy();
    Primitives2DDestroy();
    Renderer2DDestroy();
}

int OnEvent(MinimalApp* app, const MinimalEvent* e)
{
    float w, h;
    if (MinimalEventWindowSize(e, &w, &h))
    {
        SetViewport(w, h);
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    }

    switch (MinimalEventKeyPressed(e))
    {
    case GLFW_KEY_ESCAPE:    MinimalClose(app); break;
    case GLFW_KEY_F6:        MinimalToggleVsync(app); break;
    case GLFW_KEY_F7:        MinimalToggleDebug(app); break;
    case GLFW_KEY_F9:        show_info = !show_info; break;
    }

    return MINIMAL_OK;
}

void OnUpdate(MinimalApp* app, float deltatime)
{
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // render debug info
    /* fps */
    FontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", MinimalGetFps(app));

    if (show_info)
    {
        /* Settings */
        FontRendererTextFieldBegin(width - 220.0f, 8.0f, 8.0f);

        FontRendererTextFieldLine("F6: Toggle Vsync");
        FontRendererTextFieldLine("F7: Toggle debug mode");

        FontRendererTextFieldLine("F9: Toggle overlay");
    }

    FontRendererFlush();

    renderMap(&map, &tile_texture_atlas);

    Batch2DFlush();

    Primitives2DFillCircle(map.origin.x, map.origin.y, 3, IGNIS_RED);

    renderPlayer(&map, &player);

    highlightTile(&map, screenToWorld(&map, (vec2) { MinimalCursorX(), MinimalCursorY() }));

    Primitives2DFlush();
}

int main()
{
    MinimalApp app = { 
        .on_load = OnLoad,
        .on_destroy = OnDestroy,
        .on_event = OnEvent,
        .on_update = OnUpdate
    };

    if (MinimalLoad(&app, "IgnisApp", 1200, 800, "4.4"))
        MinimalRun(&app);

    MinimalDestroy(&app);

    return 0;
}