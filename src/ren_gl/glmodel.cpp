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

        static GLfloat rotation = 0;

        auto projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 500.0f);
        auto modelview = glm::lookAt(
            glm::vec3{0, 0, 60},
            glm::vec3{0, 0, 0},
            glm::vec3{0, 1, 0});
        
        modelview = glm::rotate(modelview, glm::radians(rotation), glm::vec3(0, 1, 0));
        rotation += 1.0f;

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
    auto modelHeader = (const aliashdr_t*)Mod_Extradata(const_cast<model_s*>(quakeModel));
	auto modelDesc = (const mdl_t*)((byte *)modelHeader + modelHeader->model);

    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLushort> indexes;

    for (int frameId = 0; frameId < quakeModel->numframes; ++frameId)
    {
        if (modelHeader->frames[frameId].type == ALIAS_SINGLE)
        {
    	    auto scaledVertices = (const trivertx_t*)((byte *)modelHeader + modelHeader->frames[frameId].frame);
            auto triangles = (const mtriangle_t*)((byte *)modelHeader + modelHeader->triangles);
            uint32_t vertexOffset = vertices.size() / 3;
            uint32_t indexOffset = indexes.size();
            for(int i = 0; i < modelDesc->numverts; ++i)
            {
                vertices.push_back(scaledVertices[i].v[0] * modelDesc->scale[0] + modelDesc->scale_origin[0]);
                vertices.push_back(scaledVertices[i].v[2] * modelDesc->scale[2] + modelDesc->scale_origin[2]);
                vertices.push_back(-(scaledVertices[i].v[1] * modelDesc->scale[1] + modelDesc->scale_origin[1]));
                normals.push_back(r_avertexnormals[scaledVertices[i].lightnormalindex][0]);
                normals.push_back(r_avertexnormals[scaledVertices[i].lightnormalindex][2]);
                normals.push_back(-r_avertexnormals[scaledVertices[i].lightnormalindex][1]);
            }
            for(int i = 0; i < modelDesc->numtris; ++i)
            {
                indexes.push_back(triangles[i].vertindex[0] + vertexOffset);
                indexes.push_back(triangles[i].vertindex[1] + vertexOffset);
                indexes.push_back(triangles[i].vertindex[2] + vertexOffset);
            }
            _frames.push_back(std::make_unique<SingleModelFrame>(VertexRange{indexOffset, uint32_t(indexes.size() - indexOffset), 0.0f}, modelHeader->frames[frameId].name));
        }
        else
        {
            _frames.push_back(std::make_unique<SingleModelFrame>(VertexRange{0, 0, 0.0f}, ""));
        }
    }

    _vtxBuf = std::make_unique<GLBuffer>(vertices.data(), vertices.size() * sizeof(GLfloat));
    _nrmBuf = std::make_unique<GLBuffer>(normals.data(), normals.size() * sizeof(GLfloat));
    _idxBuf = std::make_unique<GLBuffer>(indexes.data(), indexes.size() * sizeof(GLushort));

    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(kVertexInputVertex);
    _vao->format(kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputVertex, *_vtxBuf, 3 * sizeof(GLfloat));

    _vao->enableAttrib(kVertexInputNormal);
    _vao->format(kVertexInputNormal, 3, GL_FLOAT, GL_TRUE);
    _vao->vertexBuffer(kVertexInputNormal, *_nrmBuf, 3 * sizeof(GLfloat));

    _vao->indexBuffer(*_idxBuf);
}

ModelRenderer::~ModelRenderer()
{
}

void ModelRenderer::render(int frameId, float syncbase, const float* origin, const float* angles)
{
    ModelRenderProgram::setup();
    ModelRenderProgram::use();
    _vao->bind();
    frameId /= 5;
    auto vertexRange = _frames[frameId]->getVertexRange(syncbase);
    glDrawElements(GL_TRIANGLES, vertexRange.count, GL_UNSIGNED_SHORT,
        reinterpret_cast<const void*>(vertexRange.offset*sizeof(GLushort)));
    printf("%d: %s\n", frameId, _frames[frameId]->getName().c_str());
}
