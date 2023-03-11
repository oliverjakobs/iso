#include "Renderer.h"

#include <math.h>

/* ---------------------| shader |---------------------------------------------*/
static const char* vert_src = "#version 330 core \n     \
layout(location = 0) in vec2 a_Pos;                     \
layout(location = 1) in vec4 a_Color;                   \
uniform mat4 u_ViewProj;                                \
out vec4 v_Color;                                       \
void main()                                             \
{                                                       \
    gl_Position = u_ViewProj * vec4(a_Pos, 0.0, 1.0);   \
    v_Color = a_Color;                                  \
}";

static const char* frag_src = "#version 330 core \n\
layout(location = 0) out vec4 f_Color;      \
in vec4 v_Color;                            \
void main()                                 \
{                                           \
    f_Color = v_Color;                      \
}";
/* ---------------------| !shader |--------------------------------------------*/


typedef struct
{
    IgnisVertexArray vao;
    float vertices[PRIMITIVES2D_BUFFER_SIZE];
    GLsizei index;
} Primitives2D;

typedef struct
{
    Primitives2D lines;
    Primitives2D triangles;

    IgnisShader shader;
    GLint uniform_location_view_proj;
} Primitives2DStorage;

static Primitives2DStorage render_data;

static void PrimitivesDataCreate(Primitives2D* prim)
{
    IgnisBufferElement layout[] = { {GL_FLOAT, 2, GL_FALSE}, {GL_FLOAT, 4, GL_FALSE} };

    ignisGenerateVertexArray(&prim->vao);
    ignisAddArrayBufferLayout(&prim->vao, PRIMITIVES2D_BUFFER_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW, 0, layout, 2);

    prim->index = 0;
}

static void PrimitivesDataFlush(Primitives2D* prim, GLenum mode)
{
    if (prim->index == 0) return;

    ignisBindVertexArray(&prim->vao);
    ignisBufferSubData(&prim->vao.array_buffers[0], 0, prim->index * sizeof(float), prim->vertices);

    ignisUseShader(&render_data.shader);

    glDrawArrays(mode, 0, prim->index / PRIMITIVES2D_VERTEX_SIZE);

    prim->index = 0;
}

static void PrimitivesPushVertex(Primitives2D* prim, float x, float y, IgnisColorRGBA color)
{
    prim->vertices[prim->index++] = x;
    prim->vertices[prim->index++] = y;

    prim->vertices[prim->index++] = color.r;
    prim->vertices[prim->index++] = color.g;
    prim->vertices[prim->index++] = color.b;
    prim->vertices[prim->index++] = color.a;
}

void Primitives2DInit()
{
    PrimitivesDataCreate(&render_data.lines);
    PrimitivesDataCreate(&render_data.triangles);

    ignisCreateShaderSrcvf(&render_data.shader, vert_src, frag_src);
    render_data.uniform_location_view_proj = ignisGetUniformLocation(&render_data.shader, "u_ViewProj");
}

void Primitives2DDestroy()
{
    ignisDeleteVertexArray(&render_data.lines.vao);
    ignisDeleteVertexArray(&render_data.triangles.vao);
    ignisDeleteShader(&render_data.shader);
}

void Primitives2DSetViewProjection(const float* view_proj)
{
    ignisSetUniformMat4l(&render_data.shader, render_data.uniform_location_view_proj, view_proj);
}

void Primitives2DFlush()
{
    PrimitivesDataFlush(&render_data.lines, GL_LINES);
    PrimitivesDataFlush(&render_data.triangles, GL_TRIANGLES);
}

void Primitives2DRenderLine(float x0, float y0, float x1, float y1, IgnisColorRGBA color)
{
    if (render_data.lines.index + (2 * PRIMITIVES2D_VERTEX_SIZE) > PRIMITIVES2D_BUFFER_SIZE)
        PrimitivesDataFlush(&render_data.lines, GL_LINES);

    PrimitivesPushVertex(&render_data.lines, x0, y0, color);
    PrimitivesPushVertex(&render_data.lines, x1, y1, color);
}

void Primitives2DRenderRect(float x, float y, float w, float h, IgnisColorRGBA color)
{
    Primitives2DRenderLine(x + 0, y + 0, x + w, y + 0, color);
    Primitives2DRenderLine(x + w, y + 0, x + w, y + h, color);
    Primitives2DRenderLine(x + w, y + h, x + 0, y + h, color);
    Primitives2DRenderLine(x + 0, y + h, x + 0, y + 0, color);
}

void Primitives2DRenderPoly(const float* vertices, size_t count, float x, float y, IgnisColorRGBA color)
{
    if (!vertices || count < 2) return;

    float p1x = x + vertices[count - 2];
    float p1y = y + vertices[count - 1];

    size_t i = 0;
    while (i < count - 1)
    {
        float p2x = x + vertices[i++];
        float p2y = y + vertices[i++];

        Primitives2DRenderLine(p1x, p1y, p2x, p2y, color);

        p1x = p2x;
        p1y = p2y;
    }
}

void Primitives2DRenderCircle(float x, float y, float radius, IgnisColorRGBA color)
{
    float sinInc = sinf(PRIMITIVES2D_K_INCREMENT);
    float cosInc = cosf(PRIMITIVES2D_K_INCREMENT);

    float r1x = 1.0f;
    float r1y = 0.0f;

    float v1x = x + radius * r1x;
    float v1y = y + radius * r1y;

    for (int i = 0; i < PRIMITIVES2D_K_SEGMENTS; ++i)
    {
        /* Perform rotation to avoid additional trigonometry. */
        float r2x = cosInc * r1x - sinInc * r1y;
        float r2y = sinInc * r1x + cosInc * r1y;

        float v2x = x + radius * r2x;
        float v2y = y + radius * r2y;

        Primitives2DRenderLine(v1x, v1y, v2x, v2y, color);

        r1x = r2x;
        r1y = r2y;
        v1x = v2x;
        v1y = v2y;
    }
}

void Primitives2DRenderRhombus(float x, float y, float width, float height, IgnisColorRGBA color)
{
    const float vertices[] = {
        x,               y - height * .5f,
        x - width * .5f, y,
        x,               y + height * .5f,
        x + width * .5f, y
    };

    Primitives2DRenderPoly(vertices, 8, 0.0f, 0.0f, color);
}

void Primitives2DFillTriangle(float x0, float y0, float x1, float y1, float x2, float y2, IgnisColorRGBA color)
{
    if (render_data.triangles.index + (3 * PRIMITIVES2D_VERTEX_SIZE) > PRIMITIVES2D_BUFFER_SIZE)
        PrimitivesDataFlush(&render_data.triangles, GL_TRIANGLES);

    PrimitivesPushVertex(&render_data.triangles, x0, y0, color);
    PrimitivesPushVertex(&render_data.triangles, x1, y1, color);
    PrimitivesPushVertex(&render_data.triangles, x2, y2, color);
}

void Primitives2DFillRect(float x, float y, float w, float h, IgnisColorRGBA color)
{
    Primitives2DFillTriangle(x + 0, y + 0, x + w, y + 0, x + w, y + h, color);
    Primitives2DFillTriangle(x + w, y + h, x + 0, y + h, x + 0, y + 0, color);
}

void Primitives2DFillPoly(const float* vertices, size_t count, float x, float y, IgnisColorRGBA color)
{
    if (!vertices || count < 2) return;

    float x0 = x + vertices[0], y0 = y + vertices[1];
    for (size_t i = 2; i < count - 3; ++i)
    {
        float x1 = x + vertices[i + 0], y1 = y + vertices[i + 1];
        float x2 = x + vertices[i + 2], y2 = y + vertices[i + 3];
        Primitives2DFillTriangle(x0, y0, x1, y1, x2, y2, color);
    }
}

void Primitives2DFillCircle(float x, float y, float radius, IgnisColorRGBA color)
{
    float sinInc = sinf(PRIMITIVES2D_K_INCREMENT);
    float cosInc = cosf(PRIMITIVES2D_K_INCREMENT);

    float r1x = 1.0f;
    float r1y = 0.0f;

    float v1x = x + radius * r1x;
    float v1y = y + radius * r1y;

    for (int i = 0; i < PRIMITIVES2D_K_SEGMENTS; ++i)
    {
        /* Perform rotation to avoid additional trigonometry. */
        float r2x = cosInc * r1x - sinInc * r1y;
        float r2y = sinInc * r1x + cosInc * r1y;

        float v2x = x + radius * r2x;
        float v2y = y + radius * r2y;

        Primitives2DFillTriangle(x, y, v1x, v1y, v2x, v2y, color);

        r1x = r2x;
        r1y = r2y;
        v1x = v2x;
        v1y = v2y;
    }
}
