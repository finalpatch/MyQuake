#pragma once

#include "glcommon.h"

#ifdef GL45
    #include <glbinding/gl45core/gl.h>
    using namespace gl45core;
#else
    #include <glbinding/gl33core/gl.h>
    using namespace gl33core;
#endif

using GLvec3 = std::array<GLfloat, 3>;
using GLvec4 = std::array<GLfloat, 4>;
using GLmat3 = std::array<GLfloat, 3 * 3>;
using GLmat4 = std::array<GLfloat, 4 * 4>;

#ifdef GL45
    #define kGlBufferDynamic GL_DYNAMIC_STORAGE_BIT
#else
    #define kGlBufferDynamic GL_DYNAMIC_DRAW
#endif

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
#ifdef GL45
    GLBuffer(const T* data, size_t size, BufferStorageMask flags = GL_NONE_BIT)
        : _size(size)
    {
        glCreateBuffers(1, &_handle);
        glNamedBufferStorage(_handle, size * sizeof(T), data, flags);
    }
#else
    GLBuffer(const T* data, size_t size, GLenum flags = GL_STATIC_DRAW)
        : _size(size)
    {
        glGenBuffers(1, &_handle);
        bind(GL_COPY_WRITE_BUFFER);
        glBufferData(GL_COPY_WRITE_BUFFER, size * sizeof(T), data, flags);
    }
#endif
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
#ifdef GL45
        glNamedBufferSubData(_handle, offset * sizeof(T), size * sizeof(T), data);
#else
        bind(GL_COPY_WRITE_BUFFER);
        glBufferSubData(GL_COPY_WRITE_BUFFER, offset * sizeof(T), size * sizeof(T), data);
#endif
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
                assert(false);
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
            assert(false);
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
#ifndef GL45
    struct VertexFormat
    {
        GLint _size = 4;
        GLenum _type = GL_FLOAT;
        GLboolean _normalized = GL_FALSE;
        GLsizei _stride = 16;
        GLintptr _offset = 0;
    };
    std::vector<VertexFormat> _formats;
#endif
public:
    VertexArray()
    {
#ifdef GL45
        glCreateVertexArrays(1, &_handle);
#else
        glGenVertexArrays(1, &_handle);
        bind();
#endif
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
#ifdef GL45
        glEnableVertexArrayAttrib(_handle, index);
#else
        glEnableVertexAttribArray(index);
#endif
    }
    void format(GLuint index, GLint size, GLenum type, GLboolean normalized)
    {
#ifdef GL45
        glVertexArrayAttribFormat(_handle, index, size, type, normalized, 0);
#else
        if (_formats.size() <= index)
            _formats.resize(index + 1);
        _formats[index]._size = size;
        _formats[index]._type = type;
        _formats[index]._normalized = normalized;

        glVertexAttribPointer(index, _formats[index]._size, _formats[index]._type,
            _formats[index]._normalized, _formats[index]._stride,
            (const void*)_formats[index]._offset);
#endif
    }
    void vertexBuffer(GLuint index, GLGenericBuffer& buf, GLsizei stride, GLintptr offset = 0)
    {
#ifdef GL45
        glVertexArrayVertexBuffer(_handle, index, buf.handle(), offset, stride);
#else
        if (_formats.size() <= index)
            _formats.resize(index + 1);
        _formats[index]._stride = stride;
        _formats[index]._offset = offset;
        
        buf.bind(GL_ARRAY_BUFFER);
        glVertexAttribPointer(index, _formats[index]._size, _formats[index]._type,
            _formats[index]._normalized, _formats[index]._stride,
            (const void*)_formats[index]._offset);
#endif
    }
    void indexBuffer(GLGenericBuffer& buf)
    {
#ifdef GL45
        glVertexArrayElementBuffer(_handle, buf.handle());
#else
        buf.bind(GL_ELEMENT_ARRAY_BUFFER);
#endif
    }
    void bind()
    {
        glBindVertexArray(_handle);
    }
};
