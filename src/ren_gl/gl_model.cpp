#include "gl_model.h"

#include <algorithm>
#include <set>

extern "C"
{
#include "quakedef.h"
}

float r_avertexnormals[][3] = {
    #include "anorms.h"
};

extern uint32_t vid_current_palette[256];
extern glm::mat4 r_projectionMatrix;
extern glm::mat4 r_viewMatrix;

static void addVertices(const mdl_t* modelDesc, const trivertx_t* scaledVertices, const stvert_t* stverts,
    std::vector<VertexAttr>& vertexData)
{
    for(int i = 0; i < modelDesc->numverts; ++i)
    {
        VertexAttr vert{
            GLvec3{
                scaledVertices[i].v[0] * modelDesc->scale[0] + modelDesc->scale_origin[0],
                scaledVertices[i].v[2] * modelDesc->scale[2] + modelDesc->scale_origin[2],
                -(scaledVertices[i].v[1] * modelDesc->scale[1] + modelDesc->scale_origin[1])},
            GLvec3{
                r_avertexnormals[scaledVertices[i].lightnormalindex][0],
                r_avertexnormals[scaledVertices[i].lightnormalindex][2],
                -r_avertexnormals[scaledVertices[i].lightnormalindex][1]},
            GLvec2{}, // no lightmap
            GLvec2{
                (float)(stverts[i].s >> 16) / modelDesc->skinwidth,
                (float)(stverts[i].t >> 16) / modelDesc->skinheight},
            {0, 0, 0, 0}, // no light style
            stverts[i].onseam ? 1u : 0u
        };
        vertexData.push_back(std::move(vert));
    }
}

static const std::set<std::string> fullBrightObjectNames = {
    "progs/bolt.mdl",
    "progs/bolt2.mdl",
    "progs/bolt3.mdl",
    "progs/lavaball.mdl",
    "progs/missile.mdl",
    "progs/flame.mdl",
    "progs/flame2.mdl",
};

ModelRenderer::ModelRenderer(const model_s* entityModel) : _name(entityModel->name)
{
    _fullBrightObject = fullBrightObjectNames.count(_name);

    auto modelHeader = (const aliashdr_t*)Mod_Extradata(const_cast<model_s*>(entityModel));
	auto modelDesc = (const mdl_t*)((byte *)modelHeader + modelHeader->model);
    auto pstverts = (const stvert_t*)((byte *)modelHeader + modelHeader->stverts);

    // load textures
    for (int skinId = 0; skinId < modelDesc->numskins; ++skinId)
    {
        const auto* skindesc = (const maliasskindesc_t*)((byte *)modelHeader + modelHeader->skindesc) + skinId;
        if (skindesc->type == ALIAS_SKIN_GROUP)
        {
            auto skingroup = (const maliasskingroup_t *)((byte *)modelHeader + skindesc->skin);
            auto groupedSkin = std::make_unique<GroupedSkin>();
            for (int i = 0; i < skingroup->numskins; ++i)
            {
                auto intervals = (const float*)((byte*)modelHeader + skingroup->intervals);
                groupedSkin->addAnimationFrame(modelDesc->skinwidth, modelDesc->skinheight,
                    (byte *)modelHeader + skingroup->skindescs[i].skin, intervals[i]);
            }
            _skins.push_back(std::move(groupedSkin));
        }
        else
        {
            _skins.emplace_back(std::make_unique<SingleSkin>(modelDesc->skinwidth, modelDesc->skinheight,
                (byte *)modelHeader + skindesc->skin));
        }
    }

    // load vertexes
    std::vector<VertexAttr> vertexData;
    for (int frameId = 0; frameId < entityModel->numframes; ++frameId)
    {
        if (modelHeader->frames[frameId].type == ALIAS_SINGLE)
        {
    	    auto scaledVertices = (const trivertx_t*)((byte*)modelHeader + modelHeader->frames[frameId].frame);
            VertexRange vertexRange = {uint32_t(vertexData.size()), 0.0f};
            addVertices(modelDesc, scaledVertices, pstverts, vertexData);
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
                VertexRange vertexRange = {uint32_t(vertexData.size()), intervals[i]};
                addVertices(modelDesc, scaledVertices, pstverts, vertexData);
                groupedFrame->addAnimationFrame(vertexRange);
            }
            _frames.push_back(std::move(groupedFrame));
        }
    }
    _vertexBuf = std::make_unique<GLBuffer<VertexAttr, GL_ARRAY_BUFFER>>(vertexData);

    // load triangles
    std::vector<GLushort> frontIndexes;
    std::vector<GLushort> backIndexes;
    auto triangles = (const mtriangle_t*)((byte *)modelHeader + modelHeader->triangles);
    for(int i = 0; i < modelDesc->numtris; ++i)
    {
        auto& side = triangles[i].facesfront ? frontIndexes : backIndexes;
        side.insert(side.end(), triangles[i].vertindex, triangles[i].vertindex + 3);
    }
    _frontSideIdxBuf = std::make_unique<GLBuffer<GLushort, GL_ELEMENT_ARRAY_BUFFER>>(frontIndexes);
    _backSideIdxBuf = std::make_unique<GLBuffer<GLushort, GL_ELEMENT_ARRAY_BUFFER>>(backIndexes);

    // setup vertex attributes
    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(DefaultRenderPass::kVertexInputVertex);
    _vao->format(DefaultRenderPass::kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputVertex, *_vertexBuf, 0);

    _vao->enableAttrib(DefaultRenderPass::kVertexInputNormal);
    _vao->format(DefaultRenderPass::kVertexInputNormal, 3, GL_FLOAT, GL_TRUE);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputNormal, *_vertexBuf,
        offsetof(VertexAttr, normal));

    _vao->enableAttrib(DefaultRenderPass::kVertexInputDiffuseTexCoord);
    _vao->format(DefaultRenderPass::kVertexInputDiffuseTexCoord, 2, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputDiffuseTexCoord, *_vertexBuf,
        offsetof(VertexAttr, diffuseuv));

    _vao->enableAttrib(DefaultRenderPass::kVertexInputFlags);
    _vao->format(DefaultRenderPass::kVertexInputFlags, 1, GL_UNSIGNED_INT, GL_FALSE, GL_TRUE/*int*/);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputFlags, *_vertexBuf,
        offsetof(VertexAttr, vtxflags));
}

ModelRenderer::~ModelRenderer()
{
}

void ModelRenderer::render(const entity_s* entity, float ambientLight)
{
    auto frameId = entity->frame;
    auto time = entity->syncbase + cl.time;
    auto origin = entity->origin;
    auto angles = entity->angles;

    if (_fullBrightObject)
        ambientLight = 1.0;

    DefaultRenderPass::getInstance().use();

    glm::vec3 pos = qvec2glm(origin);
    glm::mat4 model =
        glm::translate(glm::mat4(), pos)
        * glm::rotate(glm::mat4(), glm::radians(angles[YAW]),   {0, 1, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[PITCH]), {0, 0, 1})
        * glm::rotate(glm::mat4(), glm::radians(angles[ROLL]),  {1, 0, 0})
        ;

    // clip agains view frustum
    auto mvp = r_projectionMatrix * r_viewMatrix * model;
    auto planes = extractViewPlanes(mvp);
    auto box = qbox2glm(entity->model->mins, entity->model->maxs);
    if (!intersectFrustumAABB(planes, box))
        return;

    const static GLfloat nullLightStyles[MAX_LIGHTSTYLES] = {};

    _vao->bind();
    auto offset = _frames[frameId]->getVertexOffset(time);

    TextureBinding diffusemapBinding(_skins[0]->getTexture(time), DefaultRenderPass::kTextureUnitDiffuse);
    // front side
    DefaultRenderPass::getInstance().setup(r_projectionMatrix, model, r_viewMatrix,
        nullLightStyles, {ambientLight, ambientLight, ambientLight, 1.0}, 0);
    _vao->indexBuffer(*_frontSideIdxBuf);
    glDrawElementsBaseVertex(GL_TRIANGLES, _frontSideIdxBuf->size(), GL_UNSIGNED_SHORT, nullptr, offset);

    // back side
    DefaultRenderPass::getInstance().use(); // need this to make it work in vmware, sigh
    DefaultRenderPass::getInstance().setup(r_projectionMatrix, model, r_viewMatrix,
        nullLightStyles, {ambientLight, ambientLight, ambientLight, 1.0}, DefaultRenderPass::kFlagBackSide);
    _vao->indexBuffer(*_backSideIdxBuf);
    glDrawElementsBaseVertex(GL_TRIANGLES, _backSideIdxBuf->size(), GL_UNSIGNED_SHORT, nullptr, offset);
}

// *******************

GroupedModelFrame::GroupedModelFrame(const char* name) : _name(name)
{
}

void GroupedModelFrame::addAnimationFrame(const VertexRange& vertexRange)
{
    _animationFrames.push_back(vertexRange);
}

uint32_t GroupedModelFrame::getVertexOffset(float time)
{
    auto fullinterval = _animationFrames.back().timestamp;
    auto targettime = time - ((int)(time / fullinterval)) * fullinterval;
    auto vertexRange = std::upper_bound(_animationFrames.begin(), _animationFrames.end(), targettime,
        [](float t, const VertexRange& r) {
            return t < r.timestamp;
        });
    return vertexRange->offset;
}

// *******************
SingleSkin::SingleSkin(GLuint w, GLuint h, const void* data) :
    _texture(GL_TEXTURE_2D, w, h, Texture::RGBA, GL_REPEAT, GL_NEAREST, GL_NEAREST)
{
    std::vector<uint32_t> rgbtex(w * h);
    const uint8_t* pixels = reinterpret_cast<const uint8_t*>(data);
    for(int i = 0; i < w * h; ++i)
        rgbtex[i] = vid_current_palette[pixels[i]];
    _texture.update(0, 0, w, h, rgbtex.data());
}

const Texture& SingleSkin::getTexture(float time)
{
    return _texture;
}

void GroupedSkin::addAnimationFrame(GLuint w, GLuint h, const void* data, float timestamp)
{
    std::vector<uint32_t> rgbtex(w * h);
    const uint8_t* pixels = reinterpret_cast<const uint8_t*>(data);
    for(int i = 0; i < w * h; ++i)
        rgbtex[i] = vid_current_palette[pixels[i]];

    TextureFrame frame = {{GL_TEXTURE_2D, w, h, Texture::RGBA, GL_REPEAT, GL_NEAREST, GL_NEAREST, rgbtex.data()},
        timestamp};
    _frames.push_back(std::move(frame));
}
const Texture& GroupedSkin::getTexture(float time)
{
    auto fullinterval = _frames.back().timestamp;
    auto targettime = time - ((int)(time / fullinterval)) * fullinterval;
    auto i = std::upper_bound(_frames.begin(), _frames.end(), targettime,
        [](float t, const TextureFrame& r) {
            return t < r.timestamp;
        });
    return i->texture;
}
