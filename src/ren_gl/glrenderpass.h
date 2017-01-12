#pragma once

#include "glhelper.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum {
    kVertexInputVertex   = 0,
    kVertexInputNormal   = 1,
    kVertexInputTexCoord = 2,
    kVertexInputStyle    = 3,
    kVertexInputTexCoord2 = 4
};

enum {
    kTextureUnitLight   = 0,
    kTextureUnitDiffuse = 1,
};

class DefaultRenderPass
{
    struct UniformBlock
    {
        GLfloat model[4*4];
        GLfloat view[4*4];
        GLfloat projection[4*4];

        GLfloat lightStyles[64];
        GLfloat ambientLight[4];
    };
public:
    static DefaultRenderPass& getInstance()
    {
        static DefaultRenderPass singleton;
        return singleton;
    }

    void setup(float w, float h, const glm::mat4& model, const glm::mat4& view,
        const GLfloat* lightStyles, const glm::vec4& ambientLight)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 5000.0f);
        UniformBlock uniformBlock;

        memcpy(uniformBlock.model, glm::value_ptr(model), sizeof(uniformBlock.model));
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));

        memcpy(uniformBlock.lightStyles, lightStyles, sizeof(uniformBlock.lightStyles));
        memcpy(uniformBlock.ambientLight, glm::value_ptr(ambientLight), sizeof(uniformBlock.ambientLight));

        _ufmBuf->update(&uniformBlock);
    }

    void use()
    {
        _prog->use();
        _prog->setUniformBlock("UniformBlock", * _ufmBuf);
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
