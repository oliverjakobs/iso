#include "Renderer.h"

typedef struct
{
    IgnisVertexArray vao;
    IgnisShader shader;

    float* vertices;
    size_t vertex_index;

    size_t quad_count;

    GLuint* texture_slots;
    size_t texture_slot_index;

    GLint uniform_location_view_proj;
} _Batch2DStorage;

static _Batch2DStorage render_data;

void Batch2DInit(const char* vert, const char* frag)
{
    ignisGenerateVertexArray(&render_data.vao);

    IgnisBufferElement layout[] =
    {
        {GL_FLOAT, 3, GL_FALSE},	/* position */
        {GL_FLOAT, 2, GL_FALSE},	/* Texture coords*/
        {GL_FLOAT, 1, GL_FALSE}		/* Texture index */
    };

    ignisAddArrayBufferLayout(&render_data.vao, BATCH2D_BUFFER_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW, 0, layout, 3);

    GLuint indices[BATCH2D_INDEX_COUNT];
    GenerateQuadIndices(indices, BATCH2D_INDEX_COUNT);

    ignisLoadElementBuffer(&render_data.vao, indices, BATCH2D_INDEX_COUNT, GL_STATIC_DRAW);

    ignisCreateShadervf(&render_data.shader, vert, frag);

    ignisUseShader(&render_data.shader);
    int samplers[BATCH2D_TEXTURES];
    for (int i = 0; i < BATCH2D_TEXTURES; i++)
        samplers[i] = i;

    ignisSetUniform1iv(&render_data.shader, "u_Textures", BATCH2D_TEXTURES, samplers);

    render_data.uniform_location_view_proj = ignisGetUniformLocation(&render_data.shader, "u_ViewProjection");

    render_data.vertices = ignisMalloc(BATCH2D_BUFFER_SIZE * sizeof(float));
    render_data.vertex_index = 0;
    render_data.quad_count = 0;

    render_data.texture_slots = ignisMalloc(BATCH2D_TEXTURES * sizeof(GLuint));
    if (render_data.texture_slots) memset(render_data.texture_slots, 0, BATCH2D_TEXTURES * sizeof(GLuint));
    render_data.texture_slot_index = 0;
}

void Batch2DDestroy()
{
    ignisFree(render_data.vertices);
    ignisFree(render_data.texture_slots);

    ignisDeleteShader(&render_data.shader);
    ignisDeleteVertexArray(&render_data.vao);
}

void Batch2DStart(const float* mat_view_proj)
{
    ignisSetUniformMat4l(&render_data.shader, render_data.uniform_location_view_proj, mat_view_proj);
}

void Batch2DFlush()
{
    if (render_data.vertex_index == 0) return;

    ignisBindVertexArray(&render_data.vao);
    ignisBufferSubData(&render_data.vao.array_buffers[0], 0, render_data.vertex_index * sizeof(float), render_data.vertices);

    ignisUseShader(&render_data.shader);

    /* bind textures */
    for (size_t i = 0; i < render_data.texture_slot_index; i++)
    {
        glActiveTexture(GL_TEXTURE0 + (GLenum)i);
        glBindTexture(GL_TEXTURE_2D, render_data.texture_slots[i]);
    }

    glDrawElements(GL_TRIANGLES, RENDERER_INDICES_PER_QUAD * (GLsizei)render_data.quad_count, GL_UNSIGNED_INT, NULL);

    render_data.vertex_index = 0;
    render_data.quad_count = 0;

    /* textures */
    memset(render_data.texture_slots, 0, BATCH2D_TEXTURES * sizeof(GLuint));
    render_data.texture_slot_index = 0;
}

static void Batch2DPushValue(float value)
{
    if (render_data.vertex_index >= BATCH2D_BUFFER_SIZE)
    {
        _ignisErrorCallback(IGNIS_WARN, "[Batch2D] Vertex overflow");
        return;
    }

    render_data.vertices[render_data.vertex_index++] = value;
}

void Batch2DRenderTexture(const IgnisTexture2D* texture, float x, float y, float w, float h)
{
    Batch2DRenderTextureSrc(texture, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);
}

void Batch2DRenderTextureFrame(const IgnisTexture2D* texture, float x, float y, float w, float h, size_t frame)
{
    float src_x, src_y, src_w, src_h;
    GetTexture2DSrcRect(texture, frame, &src_x, &src_y, &src_w, &src_h);

    Batch2DRenderTextureSrc(texture, x, y, w, h, src_x, src_y, src_w, src_h);
}

void Batch2DRenderTextureSrc(const IgnisTexture2D* texture, float x, float y, float w, float h, float src_x, float src_y, float src_w, float src_h)
{
    if (render_data.quad_count >= BATCH2D_MAX_QUADS || render_data.texture_slot_index >= BATCH2D_TEXTURES)
        Batch2DFlush();

    float texture_index = -1.0f;
    for (size_t i = 0; i < render_data.texture_slot_index; i++)
    {
        if (render_data.texture_slots[i] == texture->name)
        {
            texture_index = (float)i;
            break;
        }
    }

    if (texture_index < 0.0f)
    {
        texture_index = (float)render_data.texture_slot_index;
        render_data.texture_slots[render_data.texture_slot_index++] = texture->name;
    }

    /* BOTTOM LEFT */
    Batch2DPushValue(x);
    Batch2DPushValue(y);
    Batch2DPushValue(0.0f);

    Batch2DPushValue(src_x);
    Batch2DPushValue(src_y);

    Batch2DPushValue(texture_index);

    /* BOTTOM RIGHT */
    Batch2DPushValue(x + w);
    Batch2DPushValue(y);
    Batch2DPushValue(0.0f);

    Batch2DPushValue(src_x + src_w);
    Batch2DPushValue(src_y);

    Batch2DPushValue(texture_index);

    /* TOP RIGHT */
    Batch2DPushValue(x + w);
    Batch2DPushValue(y + h);
    Batch2DPushValue(0.0f);

    Batch2DPushValue(src_x + src_w);
    Batch2DPushValue(src_y + src_h);

    Batch2DPushValue(texture_index);

    /* TOP LEFT */
    Batch2DPushValue(x);
    Batch2DPushValue(y + h);
    Batch2DPushValue(0.0f);

    Batch2DPushValue(src_x);
    Batch2DPushValue(src_y + src_h);

    Batch2DPushValue(texture_index);

    render_data.quad_count++;
}
