#pragma once

#include "glhelper.h"

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
    void addSubFrame(const VertexRange& vertexRange);
    uint32_t getVertexOffset(float time) override;
    const std::string& getName() const override  { return _name; }
private:
    std::vector<VertexRange> _subFrames;
    std::string _name;
};

class ModelRenderer
{
public:
    ModelRenderer(const model_s* entityModel);
    virtual ~ModelRenderer();

    void render(int frameId, float time, const float* origin, const float* angles);

private:
    std::unique_ptr<VertexArray>   _vao;    // buffer setup
    std::unique_ptr<GLBuffer>      _vtxBuf; // vertex buffer
    std::unique_ptr<GLBuffer>      _nrmBuf; // normal buffer
    std::unique_ptr<GLBuffer>      _idxBuf; // index buffer

    std::vector<std::unique_ptr<ModelFrame>> _frames;
    std::string _name;
};
