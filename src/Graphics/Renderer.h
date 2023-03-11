#ifndef IGNISRENDERER_H
#define IGNISRENDERER_H

#include <stdint.h>

#include "Ignis/Ignis.h"
#include "Ignis/Quad.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RENDERER_VERTICES_PER_QUAD  4
#define RENDERER_INDICES_PER_QUAD   6

/*
 * --------------------------------------------------------------
 *                          BatchRenderer2D
 * --------------------------------------------------------------
 */
#define BATCH2D_MAX_QUADS   32
#define BATCH2D_VERTEX_SIZE (3 + 2 + 1)

#define BATCH2D_INDEX_COUNT (BATCH2D_MAX_QUADS * RENDERER_INDICES_PER_QUAD)
#define BATCH2D_BUFFER_SIZE (BATCH2D_MAX_QUADS * RENDERER_VERTICES_PER_QUAD * BATCH2D_VERTEX_SIZE)
#define BATCH2D_QUAD_SIZE   (RENDERER_VERTICES_PER_QUAD * BATCH2D_VERTEX_SIZE)

#define BATCH2D_TEXTURES    8

void Batch2DInit(const char* vert, const char* frag);
void Batch2DDestroy();

void Batch2DSetViewProjection(const float* mat_view_proj);

void Batch2DFlush();

void Batch2DRenderTexture(const IgnisTexture2D* texture, float x, float y, float w, float h);
void Batch2DRenderTextureFrame(const IgnisTexture2D* texture, float x, float y, float w, float h, uint32_t frame);

void Batch2DRenderTextureSrc(const IgnisTexture2D* texture, float x, float y, float w, float h, float src_x, float src_y, float src_w, float src_h);

/*
 * --------------------------------------------------------------
 *                          FontRenderer
 * --------------------------------------------------------------
 */

#define FONTRENDERER_MAX_QUADS      32
#define FONTRENDERER_VERTEX_SIZE    (2 + 2) /* 2f: vec; 2f: tex */

#define FONTRENDERER_INDEX_COUNT    (FONTRENDERER_MAX_QUADS * RENDERER_INDICES_PER_QUAD)
#define FONTRENDERER_BUFFER_SIZE    (FONTRENDERER_MAX_QUADS * RENDERER_VERTICES_PER_QUAD * FONTRENDERER_VERTEX_SIZE)
#define FONTRENDERER_QUAD_SIZE      (RENDERER_VERTICES_PER_QUAD * FONTRENDERER_VERTEX_SIZE)

#define FONTRENDERER_MAX_LINE_LENGTH    128

void FontRendererInit();
void FontRendererDestroy();

void FontRendererBindFont(IgnisFont* font);
void FontRendererBindFontColor(IgnisFont* font, IgnisColorRGBA color);

void FontRendererSetProjection(const float* proj);

void FontRendererFlush();

void FontRendererRenderText(float x, float y, const char* text);
void FontRendererRenderTextFormat(float x, float y, const char* fmt, ...);

void FontRendererTextFieldBegin(float x, float y, float spacing);
void FontRendererTextFieldLine(const char* fmt, ...);

/*
 * --------------------------------------------------------------
 *                          Primitives2D
 * --------------------------------------------------------------
 */

#define PRIMITIVES2D_MAX_VERTICES   3 * 1024
#define PRIMITIVES2D_VERTEX_SIZE    (2 + 4) /* 2f: position; 4f color */
#define PRIMITIVES2D_BUFFER_SIZE    (PRIMITIVES2D_VERTEX_SIZE * PRIMITIVES2D_MAX_VERTICES)

 /* Circle */
#define PRIMITIVES2D_PI             3.14159265359f
#define PRIMITIVES2D_K_SEGMENTS     36
#define PRIMITIVES2D_K_INCREMENT    2.0f * PRIMITIVES2D_PI / PRIMITIVES2D_K_SEGMENTS

void Primitives2DInit();
void Primitives2DDestroy();

void Primitives2DSetViewProjection(const float* view_proj);

void Primitives2DFlush();

void Primitives2DRenderLine(float x1, float y1, float x2, float y2, IgnisColorRGBA color);
void Primitives2DRenderRect(float x, float y, float w, float h, IgnisColorRGBA color);
void Primitives2DRenderPoly(const float* vertices, size_t count, float x, float y, IgnisColorRGBA color);
void Primitives2DRenderCircle(float x, float y, float radius, IgnisColorRGBA color);
void Primitives2DRenderRhombus(float x, float y, float width, float height, IgnisColorRGBA color);

void Primitives2DFillRect(float x, float y, float w, float h, IgnisColorRGBA color);
void Primitives2DFillPoly(const float* vertices, size_t count, float x, float y, IgnisColorRGBA color);
void Primitives2DFillCircle(float x, float y, float radius, IgnisColorRGBA color);

/*
 * --------------------------------------------------------------
 *                          Renderer2D
 * --------------------------------------------------------------
 */
void Renderer2DInit();
void Renderer2DDestroy();

void Renderer2DSetShader(IgnisShader* shader);
void Renderer2DSetViewProjection(const float* view_proj);

void Renderer2DRenderTexture(const IgnisTexture2D* texture, float x, float y);
void Renderer2DRenderTextureScale(const IgnisTexture2D* texture, float x, float y, float w, float h);
void Renderer2DRenderTextureColor(const IgnisTexture2D* texture, float x, float y, float w, float h, IgnisColorRGBA color);
void Renderer2DRenderTextureModel(const IgnisTexture2D* texture, const float* model, IgnisColorRGBA color);

/*
 * --------------------------------------------------------------
 *                          Utility
 * --------------------------------------------------------------
 */
void GenerateQuadIndices(GLuint* indices, size_t max);
void GetTexture2DSrcRect(const IgnisTexture2D* texture, uint32_t frame, float* x, float* y, float* w, float* h);

#ifdef __cplusplus
}
#endif

#endif /* !IGNISRENDERER_H */
