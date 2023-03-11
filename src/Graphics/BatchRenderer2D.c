#include "Renderer.h"

typedef struct
{
    IgnisShader shader;
    GLint uniform_location_view_proj;

    IgnisVertexArray vao;
    float vertices[BATCH2D_BUFFER_SIZE];
    uint32_t vertex_index;

    GLsizei quad_count;

    GLuint texture_slots[BATCH2D_TEXTURES];
    uint32_t texture_slot_index;
} Batch2DStorage;

static Batch2DStorage render_data;

void Batch2DInit(const char* vert, const char* frag)
{
    ignisGenerateVertexArray(&render_data.vao);

    IgnisBufferElement layout[] =
    {
        {GL_FLOAT, 3, GL_FALSE},    /* position */
        {GL_FLOAT, 2, GL_FALSE},    /* Texture coords*/
        {GL_FLOAT, 1, GL_FALSE}     /* Texture index */
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

    render_data.vertex_index = 0;
    render_data.quad_count = 0;

    render_data.texture_slot_index = 0;
}

void Batch2DDestroy()
{
    ignisDeleteShader(&render_data.shader);
    ignisDeleteVertexArray(&render_data.vao);
}

void Batch2DSetViewProjection(const float* view_proj)
{
    ignisSetUniformMat4l(&render_data.shader, render_data.uniform_location_view_proj, view_proj);
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

    glDrawElements(GL_TRIANGLES, RENDERER_INDICES_PER_QUAD * render_data.quad_count, GL_UNSIGNED_INT, NULL);

    render_data.vertex_index = 0;
    render_data.quad_count = 0;

    /* textures */
    render_data.texture_slot_index = 0;
}

void Batch2DPushVertex(float x, float y, float src_x, float src_y, uint32_t texture_index)
{
    render_data.vertices[render_data.vertex_index++] = x;
    render_data.vertices[render_data.vertex_index++] = y;
    render_data.vertices[render_data.vertex_index++] = 0.0f;

    render_data.vertices[render_data.vertex_index++] = src_x;
    render_data.vertices[render_data.vertex_index++] = src_y;

    render_data.vertices[render_data.vertex_index++] = (float)texture_index;
}

int find_texture(GLuint name, uint32_t* index)
{
    for (uint32_t i = 0; i < render_data.texture_slot_index; i++)
    {
        if (render_data.texture_slots[i] == name)
        {
            *index = i;
            return 1;
        }
    }
    return 0;
}

void Batch2DRenderTexture(const IgnisTexture2D* texture, float x, float y, float w, float h)
{
    Batch2DRenderTextureSrc(texture, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);
}

void Batch2DRenderTextureFrame(const IgnisTexture2D* texture, float x, float y, float w, float h, uint32_t frame)
{
    float src_x, src_y, src_w, src_h;
    GetTexture2DSrcRect(texture, frame, &src_x, &src_y, &src_w, &src_h);

    Batch2DRenderTextureSrc(texture, x, y, w, h, src_x, src_y, src_w, src_h);
}

void Batch2DRenderTextureSrc(const IgnisTexture2D* texture, float x, float y, float w, float h, float src_x, float src_y, float src_w, float src_h)
{
    if (render_data.vertex_index + BATCH2D_QUAD_SIZE >= BATCH2D_BUFFER_SIZE)
        Batch2DFlush();

    uint32_t texture_index = 0;
    if (!find_texture(texture->name, &texture_index))
    {
        texture_index = render_data.texture_slot_index;

        if (render_data.texture_slot_index >= BATCH2D_TEXTURES)
        {
            Batch2DFlush();
            render_data.texture_slot_index = 0; // supress warning
        }

        render_data.texture_slots[render_data.texture_slot_index++] = texture->name;
    }

    Batch2DPushVertex(x,     y,     src_x,         src_y,         texture_index);   // bottom left
    Batch2DPushVertex(x + w, y,     src_x + src_w, src_y,         texture_index);   // bottom right
    Batch2DPushVertex(x + w, y + h, src_x + src_w, src_y + src_h, texture_index);   // top right
    Batch2DPushVertex(x,     y + h, src_x,         src_y + src_h, texture_index);   // top left

    render_data.quad_count++;
}
