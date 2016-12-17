#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <fstream>
#include <sstream>
#include <memory>

#include "glhelper.h"
#include "quakeutils.h"

extern "C"
{
refdef_t r_refdef;
vec3_t	r_origin, vpn, vright, vup;
texture_s* r_notexture_mip = nullptr;
qboolean r_cache_thrash = qfalse;
}

static float r_avertexnormals[][3] = {
    #include "anorms.h"
};

struct TransformBlock
{
    glm::mat4 projection;
    glm::mat4 modelview;
};
TransformBlock transform;

std::unique_ptr<RenderProgram> prog;
std::unique_ptr<GLBuffer> uniformBuffer;
std::unique_ptr<VertexArray> vao;
std::unique_ptr<GLBuffer> vtxBuf;
std::unique_ptr<GLBuffer> nrmBuf;
std::unique_ptr<GLBuffer> idxBuf;

void R_Init (void)
{
    glbinding::Binding::initialize();
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    std::vector<Shader> shaders;
    shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs.glsl"));
    shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps.glsl"));
    prog = std::make_unique<RenderProgram>(shaders);
    uniformBuffer = std::make_unique<GLBuffer>(nullptr, sizeof(TransformBlock), GL_DYNAMIC_STORAGE_BIT);
    prog->setUniformBlock("TransformBlock", *uniformBuffer);
}
void R_InitTextures (void)
{

}
void R_InitEfrags (void)
{

}
void R_RenderView (void)
{
    GLfloat w = 1280;
    GLfloat h = 720;

    static GLfloat bgColor[] = {0.27, 0.53, 0.71, 1.0};
    glViewport(0, 0, w, h);
    glClearBufferfv(GL_COLOR, 0, bgColor);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);
    prog->use();
    
    transform.projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 500.0f);
    transform.modelview = glm::lookAt(
        glm::vec3{0, 0, 40},
        glm::vec3{0, 0, 0},
        glm::vec3{0, 1, 0});
    uniformBuffer->update(&transform, sizeof(transform), 0);
    vao->bind();
    glDrawElements(GL_TRIANGLES, idxBuf->size() / sizeof(GLushort), GL_UNSIGNED_SHORT, nullptr);
}
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect)
{

}
void R_InitSky (struct texture_s *mt)
{

}

void R_AddEfrags (entity_t *ent)
{

}
void R_RemoveEfrags (entity_t *ent)
{

}

void R_NewMap (void)
{
    auto model_desc = cl.model_precache[107];
    auto alias_model_header = (aliashdr_t *)Mod_Extradata (model_desc);
	auto alias_model = (mdl_t *)((byte *)alias_model_header + alias_model_header->model);
	auto compressed_verts = (trivertx_t *)((byte *)alias_model_header + alias_model_header->frames[0].frame);
    auto triangles = (mtriangle_t *)((byte *)alias_model_header + alias_model_header->triangles);

    std::vector<GLfloat> vertices(alias_model->numverts * 3);
    std::vector<GLfloat> normals(alias_model->numverts * 3);
    for(int i = 0; i < alias_model->numverts; ++i)
    {
        vertices[i * 3 + 0] = compressed_verts[i].v[0] * alias_model->scale[0] + alias_model->scale_origin[0];
        vertices[i * 3 + 1] = compressed_verts[i].v[2] * alias_model->scale[2] + alias_model->scale_origin[2];
        vertices[i * 3 + 2] = -(compressed_verts[i].v[1] * alias_model->scale[1] + alias_model->scale_origin[1]);
        normals[i * 3 + 0] = r_avertexnormals[compressed_verts[i].lightnormalindex][0];
        normals[i * 3 + 1] = r_avertexnormals[compressed_verts[i].lightnormalindex][2];
        normals[i * 3 + 2] = -r_avertexnormals[compressed_verts[i].lightnormalindex][1];
    }
    std::vector<GLushort> indexes(alias_model->numtris * 3);
    for(int i = 0; i < alias_model->numtris; ++i)
    {
        indexes[i * 3 + 0] = triangles[i].vertindex[0];
        indexes[i * 3 + 1] = triangles[i].vertindex[1];
        indexes[i * 3 + 2] = triangles[i].vertindex[2];
    }
    vtxBuf = std::make_unique<GLBuffer>(vertices.data(), alias_model->numverts * 3 * sizeof(GLfloat));
    nrmBuf = std::make_unique<GLBuffer>(normals.data(), alias_model->numverts * 3 * sizeof(GLfloat));
    idxBuf = std::make_unique<GLBuffer>(indexes.data(), alias_model->numtris * 3 * sizeof(short));

    vao = std::make_unique<VertexArray>();
    
    vao->enableAttrib(0);
    vao->format(0, 3, GL_FLOAT, GL_FALSE);
    vao->vertexBuffer(0, *vtxBuf, 3 * sizeof(GLfloat));
    
    vao->enableAttrib(1);
    vao->format(1, 3, GL_FLOAT, GL_TRUE);
    vao->vertexBuffer(1, *nrmBuf, 3 * sizeof(GLfloat));

    vao->indexBuffer(*idxBuf);
}

void R_ParseParticleEffect (void)
{

}
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{

}
void R_RocketTrail (vec3_t start, vec3_t end, int type)
{

}

void R_EntityParticles (entity_t *ent)
{

}
void R_BlobExplosion (vec3_t org)
{

}
void R_ParticleExplosion (vec3_t org)
{

}
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
{

}
void R_LavaSplash (vec3_t org)
{

}
void R_TeleportSplash (vec3_t org)
{

}

void R_PushDlights (void)
{

}

void D_FlushCaches (void)
{

}
void D_InitCaches (void *buffer, int size)
{

}
void R_SetVrect (vrect_t *pvrect, vrect_t *pvrectin, int lineadj)
{

}
