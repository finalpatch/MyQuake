#pragma once

#include "glhelper.h"

struct model_s;

class BrushModelRenderer
{
public:
    BrushModelRenderer(const model_s* entityModel);
    virtual ~BrushModelRenderer();

    void render(const float* origin, const float* angles);

private:
    std::unique_ptr<VertexArray>        _vao;    // buffer setup
    std::unique_ptr<GLBuffer<GLvec3>>   _vtxBuf; // vertex buffer
    std::unique_ptr<GLBuffer<GLvec3>>   _nrmBuf; // normal buffer
    std::string _name;
};
