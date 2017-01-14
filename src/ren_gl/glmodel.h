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
    virtual void bindTexture(float time) = 0;
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
    SingleSkin(int w, int h, void* data);
    void bindTexture(float time) override;
private:
    std::unique_ptr<Texture> _texture;
};

class GroupdedSkin : public Skin
{
public:
    GroupedSkin() {}
    void addAnimationFrame(int w, int h, void* data, float timestamp);
    void bindTexture(float time) override;
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
    std::unique_ptr<GLBuffer<GLushort>> _idxBuf;

    std::vector<std::unique_ptr<ModelFrame>> _frames;
    std::string _name;
};
