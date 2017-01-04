#pragma once

#include "glhelper.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum {
    kVertexInputVertex = 0,
    kVertexInputNormal = 1
};

class QuakeRenderProgram
{
    struct UniformBlock
    {
        GLfloat model[4*4];
        GLfloat view[4*4];
        GLfloat projection[4*4];
    };
public:
    static void setup(float w, float h, const glm::mat4& model, const glm::mat4& view)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 5000.0f);
        UniformBlock uniformBlock;
        memcpy(uniformBlock.model, glm::value_ptr(model), sizeof(uniformBlock.model));
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        getInstance()._ufmBuf->update(&uniformBlock);
    }
    static void use()
    {
        getInstance()._prog->use();
        getInstance()._prog->setUniformBlock("TransformBlock", * getInstance()._ufmBuf);
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

    static QuakeRenderProgram& getInstance()
    {
        static QuakeRenderProgram singleton;
        return singleton;
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock>> _ufmBuf;
};
