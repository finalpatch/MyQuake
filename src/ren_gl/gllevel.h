#pragma once

#include "glhelper.h"

struct model_s;
struct mleaf_s;
struct mnode_s;
struct efrag_s;

class LevelRenderer
{
public:
    LevelRenderer(const model_s* levelModel);
    void render();
private:
    int _visframecount = 0;
    int _framecount = 0;
    mleaf_s* _oldviewleaf;

    std::unique_ptr<VertexArray> _vao;
    std::unique_ptr<GLBuffer<GLvec3>> _vtxBuf;
    std::unique_ptr<GLBuffer<GLvec3>> _nrmBuf;
    std::unique_ptr<GLBuffer<GLuint>> _idxBuf;

    void markLeaves (mleaf_s* viewleaf);
    void storeEfrags (efrag_s **ppefrag);
    void walkBspTree(mnode_s *node, std::vector<GLuint>& indexBuffer);
};
