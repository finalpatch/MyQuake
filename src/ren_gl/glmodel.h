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
    virtual VertexRange getVertexRange(float syncbase) = 0;
    virtual const std::string& getName() const  = 0;
};

class SingleModelFrame : public ModelFrame
{
public:
    SingleModelFrame(const VertexRange& vertexRange, const char* name) :
        _vertexRange(vertexRange),
        _name(name)
    {}
    VertexRange getVertexRange(float syncbase) override { return _vertexRange; }
    const std::string& getName() const override { return _name; }
private:
    VertexRange _vertexRange;
    std::string _name;
};

class GroupedModelFrame : public ModelFrame
{
public:
    addSubFrame(const VertexRange& vertexRange);
    VertexRange getVertexRange(float syncbase) override;
    const std::string& getName() const override;
private:
    std::vector<VertexRange> _subFrames;
};

class ModelRenderer
{
public:
    ModelRenderer(const model_s* quakeModel);
    virtual ~ModelRenderer();

    void render(int frameId, float syncbase, const float* origin, const float* angles);

private:
    std::unique_ptr<VertexArray>   _vao;    // buffer setup
    std::unique_ptr<GLBuffer>      _vtxBuf; // vertex buffer
    std::unique_ptr<GLBuffer>      _nrmBuf; // normal buffer
    std::unique_ptr<GLBuffer>      _idxBuf; // index buffer

    std::vector<std::unique_ptr<ModelFrame>> _frames;
};
