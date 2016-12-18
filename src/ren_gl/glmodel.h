#pragma once

#include "glhelper.h"

struct model_s;

class ModelRenderer
{
public:
    ModelRenderer(const model_s* quakeModel);
    virtual ~ModelRenderer();

    void render(int frame, float syncbase, const float* origin, const float* angles);

protected:
    std::unique_ptr<VertexArray>   _vao;    // buffer setup
    std::unique_ptr<GLBuffer>      _vtxBuf; // vertex buffer
    std::unique_ptr<GLBuffer>      _nrmBuf; // normal buffer
    std::vector<GLBuffer>          _idxBuf; // one index buffer each frame
};
