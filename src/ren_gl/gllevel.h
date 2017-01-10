#pragma once

#include "glhelper.h"

struct model_s;
struct mleaf_s;
struct mnode_s;
struct efrag_s;

class LevelRenderer
{
public:
    struct Submodel
    {
        GLuint first;
        GLuint count;
    };

    LevelRenderer();
    
    Submodel loadBrushModel(const model_s* levelModel);
    void build();

    void renderWorld();
    void renderSubmodel(const Submodel& submodel, const float* origin, const float* angles);
private:
    int _visframecount = 0;
    int _framecount = 0;
    mleaf_s* _oldviewleaf;

    std::vector<GLfloat> _lightStyles;

    struct RendererInfo
    {
        size_t vertexIndex;
        TextureTile lightmap;
    };
    std::vector<RendererInfo> _rendererInfoArray;

    // vertex data
    std::unique_ptr<VertexArray> _vao;
    std::unique_ptr<GLBuffer<GLvec3>> _vtxBuf;
    std::unique_ptr<GLBuffer<GLvec3>> _nrmBuf;
    std::unique_ptr<GLBuffer<GLvec2>> _uvBuf;
    std::unique_ptr<GLBuffer<std::array<GLubyte, 4>>> _styBuf;
    std::unique_ptr<GLBuffer<GLuint>> _idxBuf;

    // textures
    std::unique_ptr<Texture> _lightmap;
    
    // builders
    std::vector<GLvec3> _vertexBuffer;
    std::vector<GLvec3> _normalBuffer;
    std::vector<GLvec2> _texCoordBuffer;
    std::vector<std::array<GLubyte, 4>> _styleBuffer;
    std::unique_ptr<TextureAtlasBuilder<Texture::RGBA>> _lightmapBuilder;

    void animateLight();
    void markLeaves (mleaf_s* viewleaf);
    void storeEfrags (efrag_s **ppefrag);
    void walkBspTree(mnode_s *node, std::vector<GLuint>& indexBuffer);
};
