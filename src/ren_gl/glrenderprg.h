#pragma once

#include "glhelper.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum {
    kVertexInputVertex   = 0,
    kVertexInputNormal   = 1,
    kVertexInputTexCoord = 2
};

class QuakeRenderProgram
{
    struct UniformBlock
    {
        GLfloat model[4*4];
        GLfloat view[4*4];
        GLfloat projection[4*4];

        GLfloat lightmapMask[4];
        GLfloat ambientLight[4];
    };
public:
    static QuakeRenderProgram& getInstance()
    {
        static QuakeRenderProgram singleton;
        return singleton;
    }

    void setup(float w, float h, const glm::mat4& model, const glm::mat4& view,
        const glm::vec4& lightmapMask, const glm::vec4& ambientLight)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 5000.0f);
        UniformBlock uniformBlock;

        memcpy(uniformBlock.model, glm::value_ptr(model), sizeof(uniformBlock.model));
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));

        memcpy(uniformBlock.lightmapMask, glm::value_ptr(lightmapMask), sizeof(uniformBlock.lightmapMask));
        memcpy(uniformBlock.ambientLight, glm::value_ptr(ambientLight), sizeof(uniformBlock.ambientLight));

        _ufmBuf->update(&uniformBlock);
    }
    void use()
    {
        _prog->use();
        _prog->setUniformBlock("UniformBlock", * _ufmBuf);
    }

private:
    QuakeRenderProgram()
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
