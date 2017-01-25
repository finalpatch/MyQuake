#pragma once

#include "glhelper.h"
#include "glvertexattr.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern "C" double Sys_FloatTime (void);

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

class DefaultRenderPass
{
    struct UniformBlock
    {
        GLfloat model[4 * 4];
        GLfloat view[4 * 4];
        GLfloat projection[4 * 4];

        GLfloat lightStyles[64];
        GLfloat ambientLight[4];

        GLuint flags;
        GLfloat globalTime;
        GLuint padding[2];
    };
public:
    static DefaultRenderPass &getInstance()
    {
        static DefaultRenderPass singleton;
        return singleton;
    }

    void setup(float w, float h, const glm::mat4 &model, const glm::mat4 &view,
               const GLfloat *lightStyles, const glm::vec4 &ambientLight, GLuint flags = 0)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 1.0f, 5000.0f);
        UniformBlock uniformBlock;

        memcpy(uniformBlock.model, glm::value_ptr(model), sizeof(uniformBlock.model));
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));

        memcpy(uniformBlock.lightStyles, lightStyles, sizeof(uniformBlock.lightStyles));
        memcpy(uniformBlock.ambientLight, glm::value_ptr(ambientLight), sizeof(uniformBlock.ambientLight));

        uniformBlock.flags = flags;
        uniformBlock.globalTime = fmod(Sys_FloatTime(), M_PI * 2);

        _ufmBuf->update(&uniformBlock);
    }

    void use()
    {
        _prog->use();
        _prog->setUniformBlock("UniformBlock", *_ufmBuf);
        _prog->assignTextureUnit("lightmap0", kTextureUnitLight);
        _prog->assignTextureUnit("diffusemap", kTextureUnitDiffuse);
    }

private:
    DefaultRenderPass()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);
        _ufmBuf = std::make_unique<GLBuffer<UniformBlock>>(nullptr, 1, kGlBufferDynamic);
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock>> _ufmBuf;
};

enum
{
    kTextureUnitSky0,
    kTextureUnitSky1,
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
    static SkyRenderPass &getInstance()
    {
        static SkyRenderPass singleton;
        return singleton;
    }

    void setup(float w, float h, const glm::mat4 &view, const glm::vec4& origin)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 1.0f, 5000.0f);
        UniformBlock uniformBlock;
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        memcpy(uniformBlock.origin, glm::value_ptr(origin), sizeof(uniformBlock.origin));
        uniformBlock.globalTime = fmod(Sys_FloatTime(), 8.0);
        _ufmBuf->update(&uniformBlock);
    }

    void use()
    {
        _prog->use();
        _prog->setUniformBlock("UniformBlock", *_ufmBuf);
        _prog->assignTextureUnit("skytexture0", kTextureUnitSky0);
        _prog->assignTextureUnit("skytexture1", kTextureUnitSky1);
    }

private:
    SkyRenderPass()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs_sky.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps_sky.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);
        _ufmBuf = std::make_unique<GLBuffer<UniformBlock>>(nullptr, 1, kGlBufferDynamic);
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock>> _ufmBuf;
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
    static ParticleRenderPass &getInstance()
    {
        static ParticleRenderPass singleton;
        return singleton;
    }

    void setup(float w, float h, const glm::mat4 &view, const glm::vec4& origin)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 1.0f, 5000.0f);
        UniformBlock uniformBlock;
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        memcpy(uniformBlock.origin, glm::value_ptr(origin), sizeof(uniformBlock.origin));
        _ufmBuf->update(&uniformBlock);
    }

    void use()
    {
        _prog->use();
        _prog->setUniformBlock("UniformBlock", *_ufmBuf);
    }

private:
    ParticleRenderPass()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs_particle.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps_particle.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);
        _ufmBuf = std::make_unique<GLBuffer<UniformBlock>>(nullptr, 1, kGlBufferDynamic);
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock>> _ufmBuf;
};
