#pragma once

#include "glhelper.h"
#include "glrenderpass.h"

struct model_s;

struct VertexRange
{
    uint32_t offset;
    float timestamp;
};

struct ModelFrame
{
    virtual ~ModelFrame() {}
    virtual uint32_t getVertexOffset(float time) = 0;
    virtual const std::string& getName() const  = 0;
};

struct Skin
{
    virtual ~Skin() {}
    virtual const Texture& getTexture(float time) = 0;
};

class SingleModelFrame : public ModelFrame
{
public:
    SingleModelFrame(const VertexRange& vertexRange, const char* name) :
        _vertexRange(vertexRange),
        _name(name)
    {}
    uint32_t getVertexOffset(float time) override { return _vertexRange.offset; }
    const std::string& getName() const override { return _name; }
private:
    VertexRange _vertexRange;
    std::string _name;
};

class GroupedModelFrame : public ModelFrame
{
public:
    GroupedModelFrame(const char* name);
    void addAnimationFrame(const VertexRange& vertexRange);
    uint32_t getVertexOffset(float time) override;
    const std::string& getName() const override  { return _name; }
private:
    std::vector<VertexRange> _animationFrames;
    std::string _name;
};

class SingleSkin : public Skin
{
public:
    SingleSkin(GLuint w, GLuint h, const void* data);
    const Texture& getTexture(float time) override;
private:
    Texture _texture;
};

class GroupedSkin : public Skin
{
public:
    GroupedSkin() {}
    void addAnimationFrame(GLuint w, GLuint h, const void* data, float timestamp);
    const Texture& getTexture(float time) override;
private:
    struct TextureFrame
    {
        Texture texture;
        float timestamp;
    };
    std::vector<TextureFrame> _frames;
};

class ModelRenderer
{
public:
    explicit ModelRenderer(const model_s* entityModel);
    virtual ~ModelRenderer();

    void render(int frameId, float time, const float* origin, const float* angles, float ambientLight);

private:
    std::unique_ptr<VertexArray> _vao;
    std::unique_ptr<GLBuffer<DefaultRenderPass::VertexAttr>> _vertexBuf;
    std::unique_ptr<GLBuffer<GLushort>> _frontSideIdxBuf;
    std::unique_ptr<GLBuffer<GLushort>> _backSideIdxBuf;
    std::vector<std::unique_ptr<Skin>> _skins;
    std::vector<std::unique_ptr<ModelFrame>> _frames;
    std::string _name;
};
