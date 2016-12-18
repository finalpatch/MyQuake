#include "glmodel.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern "C"
{
#include "quakedef.h"
}

static float r_avertexnormals[][3] = {
    #include "anorms.h"
};

enum {
    kVertexInputVertex = 0,
    kVertexInputNormal = 1
};

class ModelRenderProgram
{
    struct UniformBlock
    {
        GLfloat projection[4*4];
        GLfloat modelview[4*4];
    };
public:
    static void use()
    {
        getInstance()._prog->use();
    }
    
    static void setup()
    {
        GLfloat w = 1280;
        GLfloat h = 720;

        auto projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 500.0f);
        auto modelview = glm::lookAt(
            glm::vec3{0, 0, 40},
            glm::vec3{0, 0, 0},
            glm::vec3{0, 1, 0});

        UniformBlock uniformBlock;
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        memcpy(uniformBlock.modelview, glm::value_ptr(modelview), sizeof(uniformBlock.modelview));

        getInstance()._ufmBuf->update(&uniformBlock);
    }
private:
    ModelRenderProgram()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);

        _ufmBuf = std::make_unique<GLBuffer>(nullptr, sizeof(UniformBlock), GL_DYNAMIC_STORAGE_BIT);
        _prog->setUniformBlock("TransformBlock", *_ufmBuf);
    }

    static ModelRenderProgram& getInstance()
    {
        static ModelRenderProgram singleton;
        return singleton;
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer>      _ufmBuf;
};

ModelRenderer::ModelRenderer(const model_s* quakeModel)
{
    auto modelHeader = (aliashdr_t *)Mod_Extradata (const_cast<model_s*>(quakeModel));
	auto modelDesc = (mdl_t *)((byte *)modelHeader + modelHeader->model);
	auto scaledVertices = (trivertx_t *)((byte *)modelHeader + modelHeader->frames[0].frame);
    auto triangles = (mtriangle_t *)((byte *)modelHeader + modelHeader->triangles);

    std::vector<GLfloat> vertices(modelDesc->numverts * 3);
    std::vector<GLfloat> normals(modelDesc->numverts * 3);
    for(int i = 0; i < modelDesc->numverts; ++i)
    {
        vertices[i * 3 + 0] = scaledVertices[i].v[0] * modelDesc->scale[0] + modelDesc->scale_origin[0];
        vertices[i * 3 + 1] = scaledVertices[i].v[2] * modelDesc->scale[2] + modelDesc->scale_origin[2];
        vertices[i * 3 + 2] = -(scaledVertices[i].v[1] * modelDesc->scale[1] + modelDesc->scale_origin[1]);
        normals[i * 3 + 0] = r_avertexnormals[scaledVertices[i].lightnormalindex][0];
        normals[i * 3 + 1] = r_avertexnormals[scaledVertices[i].lightnormalindex][2];
        normals[i * 3 + 2] = -r_avertexnormals[scaledVertices[i].lightnormalindex][1];
    }
    std::vector<GLushort> indexes(modelDesc->numtris * 3);
    for(int i = 0; i < modelDesc->numtris; ++i)
    {
        indexes[i * 3 + 0] = triangles[i].vertindex[0];
        indexes[i * 3 + 1] = triangles[i].vertindex[1];
        indexes[i * 3 + 2] = triangles[i].vertindex[2];
    }
    _vtxBuf = std::make_unique<GLBuffer>(vertices.data(), modelDesc->numverts * 3 * sizeof(GLfloat));
    _nrmBuf = std::make_unique<GLBuffer>(normals.data(), modelDesc->numverts * 3 * sizeof(GLfloat));
    _idxBuf.emplace_back(indexes.data(), modelDesc->numtris * 3 * sizeof(GLushort));

    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(kVertexInputVertex);
    _vao->format(kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputVertex, *_vtxBuf, 3 * sizeof(GLfloat));

    _vao->enableAttrib(kVertexInputNormal);
    _vao->format(kVertexInputNormal, 3, GL_FLOAT, GL_TRUE);
    _vao->vertexBuffer(kVertexInputNormal, *_nrmBuf, 3 * sizeof(GLfloat));

    _vao->indexBuffer(_idxBuf[0]);
}

ModelRenderer::~ModelRenderer()
{
}

void ModelRenderer::render(int frame, float syncbase, const float* origin, const float* angles)
{
    ModelRenderProgram::setup();
    ModelRenderProgram::use();
    _vao->bind();
    glDrawElements(GL_TRIANGLES, _idxBuf[0].size() / sizeof(GLushort), GL_UNSIGNED_SHORT, nullptr);
}
