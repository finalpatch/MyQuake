#include "glmodel.h"
#include "glrenderpass.h"

#include <algorithm>

extern "C"
{
#include "quakedef.h"
}

static float r_avertexnormals[][3] = {
    #include "anorms.h"
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

    _vtxBuf = std::make_unique<GLBuffer<GLvec3>>(vertices);
    _nrmBuf = std::make_unique<GLBuffer<GLvec3>>(normals);
    _idxBuf = std::make_unique<GLBuffer<GLushort>>(indexes);

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

void ModelRenderer::render(int frameId, float time, const float* origin, const float* angles, float ambientLight)
{
    glm::vec3 eyePos = qvec2glm(r_origin);
    glm::vec3 eyeDirection = qvec2glm(vpn);
    glm::vec3 pos = qvec2glm(origin);

    glm::mat4 model =
        glm::translate(glm::mat4(), pos)
        * glm::rotate(glm::mat4(), glm::radians(angles[YAW]),   {0, 1, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[PITCH]), {0, 0, 1})
        * glm::rotate(glm::mat4(), glm::radians(angles[ROLL]),  {1, 0, 0})
        ;
    glm::mat4 view = glm::lookAt(eyePos, eyePos + eyeDirection, qvec2glm(vup));

    const static GLfloat nullLightStyles[MAX_LIGHTSTYLES] = {};

    DefaultRenderPass::getInstance().setup(vid.width, vid.height, model, view,
        nullLightStyles, {ambientLight, ambientLight, ambientLight, 1.0});
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
