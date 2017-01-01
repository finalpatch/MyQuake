#include "glmodel.h"

#include <algorithm>

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
        GLfloat model[4*4];
        GLfloat view[4*4];
        GLfloat projection[4*4];
    };
public:
    static void setup(float w, float h, const glm::mat4& model, const glm::mat4& view)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 5000.0f);
        UniformBlock uniformBlock;
        memcpy(uniformBlock.model, glm::value_ptr(model), sizeof(uniformBlock.model));
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        getInstance()._ufmBuf->update(&uniformBlock);
    }
    static void use()
    {
        getInstance()._prog->use();
        getInstance()._prog->setUniformBlock("TransformBlock", * getInstance()._ufmBuf);
    }

private:
    ModelRenderProgram()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);
        _ufmBuf = std::make_unique<GLBuffer<UniformBlock>>(nullptr, 1, GL_DYNAMIC_STORAGE_BIT);
    }

    static ModelRenderProgram& getInstance()
    {
        static ModelRenderProgram singleton;
        return singleton;
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock>> _ufmBuf;
};

static void addVertices(const mdl_t* modelDesc, const trivertx_t* scaledVertices,
    std::vector<GLvec3>& vertices, std::vector<GLvec3>& normals)
{
    for(int i = 0; i < modelDesc->numverts; ++i)
    {
        vertices.emplace_back(GLvec3{
            scaledVertices[i].v[0] * modelDesc->scale[0] + modelDesc->scale_origin[0],
            scaledVertices[i].v[2] * modelDesc->scale[2] + modelDesc->scale_origin[2],
            -(scaledVertices[i].v[1] * modelDesc->scale[1] + modelDesc->scale_origin[1])});
        normals.emplace_back(GLvec3{
            r_avertexnormals[scaledVertices[i].lightnormalindex][0],
            r_avertexnormals[scaledVertices[i].lightnormalindex][2],
            -r_avertexnormals[scaledVertices[i].lightnormalindex][1]});
    }
}

ModelRenderer::ModelRenderer(const model_s* entityModel) : _name(entityModel->name)
{
    auto modelHeader = (const aliashdr_t*)Mod_Extradata(const_cast<model_s*>(entityModel));
	auto modelDesc = (const mdl_t*)((byte *)modelHeader + modelHeader->model);

    std::vector<GLvec3> vertices;
    std::vector<GLvec3> normals;
    std::vector<GLushort> indexes;

    for (int frameId = 0; frameId < entityModel->numframes; ++frameId)
    {
        if (modelHeader->frames[frameId].type == ALIAS_SINGLE)
        {
    	    auto scaledVertices = (const trivertx_t*)((byte*)modelHeader + modelHeader->frames[frameId].frame);
            VertexRange vertexRange = {uint32_t(vertices.size()), 0.0f};
            addVertices(modelDesc, scaledVertices, vertices, normals);
            _frames.push_back(std::make_unique<SingleModelFrame>(vertexRange, modelHeader->frames[frameId].name));
        }
        else
        {
            auto groupedFrame = std::make_unique<GroupedModelFrame>(modelHeader->frames[frameId].name);

            auto group = (const maliasgroup_t*)((byte*)modelHeader + modelHeader->frames[frameId].frame);
            auto intervals = (const float*)((byte*)modelHeader + group->intervals);
            auto numframes = group->numframes;

            for (int i = 0; i < numframes; ++i)
            {
                auto scaledVertices = (const trivertx_t*)((byte*)modelHeader + group->frames[i].frame);
                VertexRange vertexRange = {uint32_t(vertices.size()), intervals[i]};
                addVertices(modelDesc, scaledVertices, vertices, normals);
                groupedFrame->addSubFrame(vertexRange);
            }
            _frames.push_back(std::move(groupedFrame));
        }
    }

    auto triangles = (const mtriangle_t*)((byte *)modelHeader + modelHeader->triangles);
    for(int i = 0; i < modelDesc->numtris; ++i)
    {
        indexes.push_back(triangles[i].vertindex[0]);
        indexes.push_back(triangles[i].vertindex[1]);
        indexes.push_back(triangles[i].vertindex[2]);
    }

    _vtxBuf = std::make_unique<GLBuffer<GLvec3>>(vertices.data(), vertices.size());
    _nrmBuf = std::make_unique<GLBuffer<GLvec3>>(normals.data(), normals.size());
    _idxBuf = std::make_unique<GLBuffer<GLushort>>(indexes.data(), indexes.size());

    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(kVertexInputVertex);
    _vao->format(kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputVertex, *_vtxBuf, sizeof(GLvec3));

    _vao->enableAttrib(kVertexInputNormal);
    _vao->format(kVertexInputNormal, 3, GL_FLOAT, GL_TRUE);
    _vao->vertexBuffer(kVertexInputNormal, *_nrmBuf, sizeof(GLvec3));

    _vao->indexBuffer(*_idxBuf);
}

ModelRenderer::~ModelRenderer()
{
}

void ModelRenderer::beginRenderModels()
{
    ModelRenderProgram::use();
}

void ModelRenderer::endRenderModels()
{
}

void ModelRenderer::render(int frameId, float time, const float* origin, const float* angles)
{
    glm::vec3 eyePos = qvec2glm(r_origin);
    glm::vec3 eyeDirection = qvec2glm(vpn);
    glm::vec3 pos = qvec2glm(origin);
    glm::mat4 model = 
        glm::translate(glm::mat4(), pos)
        * glm::rotate(glm::mat4(), glm::radians(angles[2]), {0, 0, 1})
        * glm::rotate(glm::mat4(), glm::radians(angles[1]), {0, 1, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[0]), {1, 0, 0});
    glm::mat4 view = glm::lookAt(eyePos, eyePos + eyeDirection, qvec2glm(vup));

    ModelRenderProgram::setup(vid.width, vid.height, model, view);
    _vao->bind();
    auto offset = _frames[frameId]->getVertexOffset(time);
    glDrawElementsBaseVertex(GL_TRIANGLES, _idxBuf->size(), GL_UNSIGNED_SHORT, nullptr, offset);
}

// *******************

GroupedModelFrame::GroupedModelFrame(const char* name) : _name(name)
{
}

void GroupedModelFrame::addSubFrame(const VertexRange& vertexRange)
{
    _subFrames.push_back(vertexRange);
}

uint32_t GroupedModelFrame::getVertexOffset(float time)
{
    auto fullinterval = _subFrames.back().timestamp;
    auto targettime = time - ((int)(time / fullinterval)) * fullinterval;
    auto vertexRange = std::upper_bound(_subFrames.begin(), _subFrames.end(), targettime,
        [](float t, const VertexRange& r) {
            return t < r.timestamp;
        });
    return vertexRange->offset;
}
