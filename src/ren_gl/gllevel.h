#pragma once

#include "glhelper.h"

struct model_s;
struct mleaf_s;

class LevelRenderer
{
public:
    LevelRenderer(const model_s* levelModel);
    void render();
private:
    int _visframecount;
    mleaf_s* _oldviewleaf;

    std::unique_ptr<VertexArray> _vao;
    std::unique_ptr<GLBuffer> _vtxBuf;
    std::unique_ptr<GLBuffer> _nrmBuf;
    std::unique_ptr<GLBuffer> _idxBuf;

    void markLeaves (mleaf_s* viewleaf);
};
