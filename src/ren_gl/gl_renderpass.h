#pragma once

#include "gl_helpers.h"
#include "gl_vertexattr.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern "C" double Sys_FloatTime (void);
#define MAX_DLIGHTS 32

class DefaultRenderPass
{
    struct UniformBlock
    {
        GLfloat model[4 * 4];
        GLfloat view[4 * 4];
        GLfloat projection[4 * 4];

        GLfloat lightStyles[64];
        GLfloat ambientLight[4];

        GLfloat dlights[MAX_DLIGHTS * 4]; // xyz, radius
        GLuint ndlights;

        GLuint flags;
        GLfloat globalTime;
        GLuint padding[1];
    };
public:
    enum
    {
        kVertexInputVertex,
        kVertexInputNormal,
        kVertexInputLightTexCoord,
        kVertexInputDiffuseTexCoord,
        kVertexInputLightStyle,
        kVertexInputFlags
    };
    enum
    {
        kTextureUnitLight,
        kTextureUnitDiffuse,
    };
    enum
    {
        kFlagBackSide = 1,
        kFlagTurbulence = 2
    };
    static DefaultRenderPass &getInstance()
    {
        static DefaultRenderPass singleton;
        return singleton;
    }

    void updateDLights(const GLfloat* dlights, GLuint ndlights)
    {
        memcpy(_uniformBlock.dlights, dlights, sizeof(GLfloat)*ndlights*4);
        _uniformBlock.ndlights = ndlights;
    }

    void setup(const glm::mat4& projection, const glm::mat4 &model, const glm::mat4 &view, const GLfloat *lightStyles,
        const glm::vec4 &ambientLight, GLuint flags = 0)
    {
        memcpy(_uniformBlock.model, glm::value_ptr(model), sizeof(_uniformBlock.model));
        memcpy(_uniformBlock.view, glm::value_ptr(view), sizeof(_uniformBlock.view));
        memcpy(_uniformBlock.projection, glm::value_ptr(projection), sizeof(_uniformBlock.projection));

        memcpy(_uniformBlock.lightStyles, lightStyles, sizeof(_uniformBlock.lightStyles));
        memcpy(_uniformBlock.ambientLight, glm::value_ptr(ambientLight), sizeof(_uniformBlock.ambientLight));

        _uniformBlock.flags = flags;
        _uniformBlock.globalTime = fmod(Sys_FloatTime(), M_PI * 2);

        _ufmBuf->update(&_uniformBlock);
    }

    void use()
    {
        _prog->use();
        _prog->setUniformBlock(_uniformBlockIndex, *_ufmBuf);
    }

private:
    DefaultRenderPass()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);
        _prog->use();
        _ufmBuf = std::make_unique<GLBuffer<UniformBlock, GL_UNIFORM_BUFFER>>(nullptr, 1, kGlBufferDynamic);
        _uniformBlockIndex = _prog->getUniformBlockIndex("UniformBlock");
        _prog->assignTextureUnit("lightmap0", kTextureUnitLight);
        _prog->assignTextureUnit("diffusemap", kTextureUnitDiffuse);
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock, GL_UNIFORM_BUFFER>> _ufmBuf;
    UniformBlock _uniformBlock;
    GLuint _uniformBlockIndex;
};

class SkyRenderPass
{
    struct UniformBlock
    {
        GLfloat view[4 * 4];
        GLfloat projection[4 * 4];
        GLfloat origin[4];
        GLfloat globalTime;
        GLuint padding[3];
    };
public:
    // Sky render pass share the same vertex layout as DefaultRenderPass
    enum
    {
        kTextureUnitSky0,
        kTextureUnitSky1,
    };
    static SkyRenderPass &getInstance()
    {
        static SkyRenderPass singleton;
        return singleton;
    }

    void setup(const glm::mat4& projection, const glm::mat4 &view, const glm::vec4& origin)
    {
        UniformBlock uniformBlock;
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        memcpy(uniformBlock.origin, glm::value_ptr(origin), sizeof(uniformBlock.origin));
        uniformBlock.globalTime = fmod(Sys_FloatTime(), 8.0);
        _ufmBuf->update(&uniformBlock);
        _uniformBlockIndex = _prog->getUniformBlockIndex("UniformBlock");
    }

    void use()
    {
        _prog->use();
        _prog->setUniformBlock(_uniformBlockIndex, *_ufmBuf);
    }

private:
    SkyRenderPass()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs_sky.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps_sky.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);
        _prog->use();
        _ufmBuf = std::make_unique<GLBuffer<UniformBlock, GL_UNIFORM_BUFFER>>(nullptr, 1, kGlBufferDynamic);
        _prog->assignTextureUnit("skytexture0", kTextureUnitSky0);
        _prog->assignTextureUnit("skytexture1", kTextureUnitSky1);
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock, GL_UNIFORM_BUFFER>> _ufmBuf;
    GLuint _uniformBlockIndex;
};

class ParticleRenderPass
{
    struct UniformBlock
    {
        GLfloat view[4 * 4];
        GLfloat projection[4 * 4];
        GLfloat origin[4];
    };
public:
    enum
    {
        kVertexInputVertex,
        kVertexInputColor
    };
    static ParticleRenderPass &getInstance()
    {
        static ParticleRenderPass singleton;
        return singleton;
    }

    void setup(const glm::mat4& projection, const glm::mat4 &view, const glm::vec4& origin)
    {
        UniformBlock uniformBlock;
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        memcpy(uniformBlock.origin, glm::value_ptr(origin), sizeof(uniformBlock.origin));
        _ufmBuf->update(&uniformBlock);
    }

    void use()
    {
        _prog->use();
        _prog->setUniformBlock(_uniformBlockIndex, *_ufmBuf);
    }

private:
    ParticleRenderPass()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs_particle.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps_particle.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);
        _ufmBuf = std::make_unique<GLBuffer<UniformBlock, GL_UNIFORM_BUFFER>>(nullptr, 1, kGlBufferDynamic);
        _uniformBlockIndex = _prog->getUniformBlockIndex("UniformBlock");
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock, GL_UNIFORM_BUFFER>> _ufmBuf;
    GLuint _uniformBlockIndex;
};
