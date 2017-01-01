#pragma once

#include <glbinding/gl45core/gl.h>

#include "glcommon.h"

using namespace gl45core;

using GLvec3 = std::array<GLfloat, 3>;
using GLvec4 = std::array<GLfloat, 4>;
using GLmat3 = std::array<GLfloat, 3 * 3>;
using GLmat4 = std::array<GLfloat, 4 * 4>;

class GLObject
{
public:
    GLObject() : _handle(0)
    {
    }
    GLObject(GLObject&& other)
    {
        _handle = other._handle;
        other._handle = 0;
    }

    GLObject(const GLObject&) = delete;

    virtual ~GLObject() {}

    GLuint handle() const
    {
        return _handle;
    }
protected:
    GLuint _handle;
};

class GLGenericBuffer : public GLObject
{
public:
    void bind(GLenum target)
    {
        glBindBuffer(target, _handle);
    }
    void bind(GLenum target, GLuint index)
    {
        glBindBufferBase(target, index, _handle);
    }
};

template<typename T>
class GLBuffer : public GLGenericBuffer
{
    size_t _size;
public:
    GLBuffer(const T* data, size_t size, GLenum flags = GL_STATIC_DRAW)
        : _size(size)
    {
        //glCreateBuffers(1, &_handle);
        //glNamedBufferStorage(_handle, size * sizeof(T), data, flags);
        glGenBuffers(1, &_handle);
        bind(GL_COPY_WRITE_BUFFER);
        glBufferData(GL_COPY_WRITE_BUFFER, size * sizeof(T), data, flags);
    }
    GLBuffer(GLBuffer&& other) : GLObject(std::move(other))
    {
        _size = other.size();
    }
    ~GLBuffer() override
    {
        if (_handle)
            glDeleteBuffers(1, &_handle);
    }
    size_t size() const
    {
        return _size;
    }
    void update(const T* data, size_t size, size_t offset = 0)
    {
        //glNamedBufferSubData(_handle, offset * sizeof(T), size * sizeof(T), data);
        bind(GL_COPY_WRITE_BUFFER);
        glBufferSubData(GL_COPY_WRITE_BUFFER, offset * sizeof(T), size * sizeof(T), data);
    }
    void update(const T* data)
    {
        update(data, 1);
    }
};

class Shader : public GLObject
{
public:
    Shader(GLenum type, const std::string& src)
    {
        if (!src.empty())
        {
            _handle = glCreateShader(type);
            const char* srcList[] = {src.c_str()};
            glShaderSource(_handle, 1, srcList, nullptr);
            glCompileShader(_handle);
            GLint compilationOk;
            glGetShaderiv(_handle, GL_COMPILE_STATUS, &compilationOk);
            if (!compilationOk)
            {
                printf("shader compilation failed\n");
                GLint errLength;
                glGetShaderiv(_handle, GL_INFO_LOG_LENGTH, &errLength);
                std::vector<char> errLog(errLength+1, '\0');
                glGetShaderInfoLog(_handle, errLength, &errLength, errLog.data());
                printf(errLog.data());
            }
        }
    }
    Shader(Shader&& other) : GLObject(std::move(other))
    {
    }
    ~Shader() override
    {
        if (_handle)
            glDeleteShader(_handle);
    }
};

class RenderProgram : public GLObject
{
public:
    RenderProgram(const std::vector<Shader>& shaders)
    {
        _handle = glCreateProgram();
        for(const auto& shader: shaders)
        {
            glAttachShader(_handle, shader.handle());
        }
        glLinkProgram(_handle);
        GLint isLinked = 0;
        glGetProgramiv(_handle, GL_LINK_STATUS, &isLinked);
        if(!isLinked)
        {
            GLint maxLength;
            glGetProgramiv(_handle, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(_handle, maxLength, &maxLength, &infoLog[0]);
            printf("link failed: %s", infoLog.data());
        }
    }
    RenderProgram(RenderProgram&& other) : GLObject(std::move(other))
    {
    }
    ~RenderProgram() override
    {
        if (_handle)
            glDeleteProgram(_handle);
    }
    void use() const
    {
        glUseProgram(_handle);
    }
    void setUniformBlock(const std::string& name, GLGenericBuffer& buf)
    {
        GLuint idx = glGetUniformBlockIndex(_handle, name.c_str());
        if (idx != GL_INVALID_INDEX)
            buf.bind(GL_UNIFORM_BUFFER, idx);
        else
            printf("invalid uniform name\n");
    }
};

class VertexArray : public GLObject
{
    GLint _size;
    GLenum _type;
    GLboolean _normalized;
public:
    VertexArray()
    {
        //glCreateVertexArrays(1, &_handle);
        glGenVertexArrays(1, &_handle);
        bind();
    }
    VertexArray(VertexArray&& other) : GLObject(std::move(other))
    {
    }
    ~VertexArray() override
    {
        if (_handle)
            glDeleteVertexArrays(1, &_handle);
    }
    void enableAttrib(GLuint index)
    {
        // glEnableVertexArrayAttrib(_handle, index);
        glEnableVertexAttribArray(index);
    }
    void format(GLuint index, GLint size, GLenum type, GLboolean normalized, GLuint offset = 0)
    {
        // glVertexArrayAttribFormat(_handle, index, size, type, normalized, offset);
        _size = size;
        _type = type;
        _normalized = normalized;
    }
    void vertexBuffer(GLuint index, GLGenericBuffer& buf, GLsizei stride, GLintptr offset = 0)
    {
        // glVertexArrayVertexBuffer(_handle, index, buf.handle(), offset, stride);
        buf.bind(GL_ARRAY_BUFFER);
        glVertexAttribPointer(index, _size, _type, _normalized, stride, nullptr);
    }
    void indexBuffer(GLGenericBuffer& buf)
    {
        // glVertexArrayElementBuffer(_handle, buf.handle());
        buf.bind(GL_ELEMENT_ARRAY_BUFFER);
    }
    void bind()
    {
        glBindVertexArray(_handle);
    }
};
