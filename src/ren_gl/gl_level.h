#pragma once

#include "gl_renderpass.h"

struct model_s;
struct mleaf_s;
struct mnode_s;
struct efrag_s;
struct texture_s;
struct entity_s;
struct msurface_s;

class LevelRenderer
{
public:
    LevelRenderer();

    void loadBrushModel(const model_s* levelModel);
    void build();

    void renderWorld(const entity_s* entity);
    void renderSubmodel(const entity_s* entity);

    float lightPoint (const float* p);
private:
    int _visframecount = 0;
    int _framecount = 0;
    mleaf_s* _oldviewleaf;

    std::vector<GLfloat> _lightStyles;

    // vertex data
    std::unique_ptr<VertexArray> _vao;
    std::unique_ptr<GLBuffer<VertexAttr>> _vertexBuf;
    std::unique_ptr<GLBuffer<GLuint>> _idxBuf;

    // textures
    std::unique_ptr<Texture> _lightmap;
    struct TextureChain
    {
        Texture texture;
        std::vector<GLuint> vertexes;

        TextureChain(GLuint width, GLuint height) :
            texture(GL_TEXTURE_2D, width, height, Texture::RGBA,
                GL_REPEAT, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST)
        {}
    };
    std::vector<TextureChain> _diffuseTextureChains;
    std::vector<TextureChain> _turbulenceTextureChains;
    std::vector<TextureChain> _skyTextureChains;
    std::vector<Texture> _skyBackgroundTextures;

    // builders
    std::vector<VertexAttr> _vertexData;
    std::unique_ptr<TextureAtlasBuilder<Texture::RGBA>> _lightmapBuilder;

    void animateLight();
    void markLeaves (mleaf_s* viewleaf);
    void storeEfrags (efrag_s **ppefrag);
    void walkBspTree(mnode_s *node, const entity_s* entity);
    int recursiveLightPoint (mnode_s* node, const float* start, const float* end);
    void emitSurface(const msurface_s& surf, int frame);
    void renderTextureChains(const glm::mat4& modelMatrix);

    void loadTexture(texture_s* texture);
    void loadSkyTexture(texture_s* texture);
    const texture_s* textureAnimation(const texture_s* base, int frame);
};
