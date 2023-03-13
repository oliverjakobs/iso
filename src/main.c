#include <Ignis/Ignis.h>
#include <Ignis/Renderer/Renderer.h>

#include "Minimal/Application.h"

#include "iso.h"

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

uint32_t grid[] = {
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


typedef struct
{
    vec2 position;
    float speed;
} Player;


void renderPlayer(const IsoMap* map, const Player* player)
{
    vec2 screen = worldToScreen(map, player->position);
    ignisPrimitives2DFillCircle(screen.x, screen.y, 3, IGNIS_BLACK);
}

static void SetViewport(float w, float h)
{
    width = w;
    height = h;
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);

    ignisRenderer2DSetViewProjection(screen_projection.v);
    ignisPrimitives2DSetViewProjection(screen_projection.v);
    ignisFontRendererSetProjection(screen_projection.v);
    ignisBatch2DSetViewProjection(screen_projection.v);
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
    ignisPrimitives2DInit();
    ignisFontRendererInit();
    ignisRenderer2DInit();
    ignisBatch2DInit("res/shaders/batch.vert", "res/shaders/batch.frag");

    SetViewport((float)w, (float)h);

    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    MINIMAL_INFO("[GLFW] Version:        %s", glfwGetVersionString());
    MINIMAL_INFO("[OpenGL] Version:      %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL] Vendor:       %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL] Renderer:     %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL] GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis] Version:       %s", ignisGetVersionString());

    ignisCreateTexture2D(&tile_texture_atlas, "res/tiles.png", 1, 4, 0, NULL);

    isoMapInit(&map, grid, 10, 10, 50, 8.0f);
    isoMapSetOrigin(&map, (vec2) { width * 0.5f, 100.0f });

    player.position = (vec2){ 5 * map.tile_size, 5 * map.tile_size };
    player.speed = 60.0f;

    return MINIMAL_OK;
}

void OnDestroy(MinimalApp* app)
{
    ignisDeleteFont(&font);

    ignisBatch2DDestroy();
    ignisFontRendererDestroy();
    ignisPrimitives2DDestroy();
    ignisRenderer2DDestroy();
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
    vec2 velocity;
    velocity.x = (-MinimalKeyDown(GLFW_KEY_A) + MinimalKeyDown(GLFW_KEY_D)) * player.speed;
    velocity.y = (-MinimalKeyDown(GLFW_KEY_W) + MinimalKeyDown(GLFW_KEY_S)) * player.speed;

    velocity = vec2_normalize(isoToCartesian(velocity));

    player.position = vec2_add(player.position, vec2_mult(velocity, deltatime));

    // clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // render debug info
    /* fps */
    ignisFontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", MinimalGetFps(app));

    if (show_info)
    {
        /* Settings */
        ignisFontRendererTextFieldBegin(width - 220.0f, 8.0f, 8.0f);

        ignisFontRendererTextFieldLine("F6: Toggle Vsync");
        ignisFontRendererTextFieldLine("F7: Toggle debug mode");

        ignisFontRendererTextFieldLine("F9: Toggle overlay");
    }

    ignisFontRendererFlush();

    renderMap(&map, &tile_texture_atlas);

    ignisBatch2DFlush();

    ignisPrimitives2DFillCircle(map.origin.x, map.origin.y, 3, IGNIS_RED);

    renderPlayer(&map, &player);

    highlightTile(&map, screenToWorld(&map, (vec2) { MinimalCursorX(), MinimalCursorY() }));

    ignisPrimitives2DFlush();
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