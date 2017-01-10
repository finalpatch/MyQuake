#pragma once

#include "glcommon.h"
#include <algorithm>

#ifdef GL45
    #include <glbinding/gl45core/gl.h>
    using namespace gl45core;
#else
    #include <glbinding/gl33core/gl.h>
    using namespace gl33core;
#endif

using GLvec2 = std::array<GLfloat, 2>;
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
    VertexArray(VertexArray&& other) :
        GLObject(std::move(other)),
        _formats(std::move(other._formats))
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
        glVertexArrayAttribBinding(_handle, index, index);
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

class Texture : public GLObject
{
    GLenum _target;
    GLint _internalFormat;
    GLenum _format;
    GLenum _datatype;
    GLuint _width;
    GLuint _height;
public:
    enum Type
    {
        RGBA,
        GRAY
    };
    Texture(GLenum target, GLuint width, GLuint height, Type type, const GLvoid* data = nullptr)
    {
        _target = target;
        glGenTextures(1, &_handle);
        if (type == RGBA)
        {
            _internalFormat = (GLint)GL_RGBA8; //GL_SRGB8_ALPHA8
            _format = GL_RGBA;
            _datatype = GL_UNSIGNED_INT_8_8_8_8;
        }
        else
        {
            _internalFormat = (GLint)GL_R8;
            _format = GL_RED;
            _datatype = GL_UNSIGNED_BYTE;
        }
        _width = width;
        _height = height;
        
        glBindTexture(_target, _handle);
        glTexImage2D(_target, 0, _internalFormat, width, height, 0, _format, _datatype, data);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    Texture(Texture&& other) : GLObject(std::move(other))
    {
        _target = other._target;
        _internalFormat = other._internalFormat;
        _format = other._format;
        _datatype = other._datatype;
        _width = other._width;
        _height = other._height;
    }
    ~Texture() override
    {
        if (_handle)
            glDeleteTextures(1, &_handle);
    }
    GLenum target() const
    {
        return _target;
    }
    GLuint width() const
    {
        return _width;
    }
    GLuint height() const
    {
        return _height;
    }
    void update(GLint x, GLint y, GLuint w, GLuint h, const GLvoid* data)
    {
        glBindTexture(_target, _handle);
        glTexSubImage2D(_target, 0, x, y, w, h, _format, _datatype, data);
    }
    void update(const GLvoid* data)
    {
        update(0, 0, _width, _height, data);
    }
};

class TextureBinding
{
    const GLenum _boundUnit;
    const Texture& _texture;
public:
    TextureBinding(const Texture& texture, GLenum textureUnit = GL_TEXTURE0) :
        _boundUnit(textureUnit), _texture(texture)
    {
        glActiveTexture(textureUnit);
        glBindTexture(texture.target(), texture.handle());
    }
    TextureBinding(const TextureBinding&) = delete;
    ~TextureBinding()
    {
        glActiveTexture(_boundUnit);
        glBindTexture(_texture.target(), 0);
    }
};

template<typename T>
class Image
{
    const GLuint _w, _h;
    std::vector<T> _data;
public:
    Image(GLuint width, GLuint height, const T* data) :
        _w(width), _h(height), _data(_w * _h)
    {
        if (data)
            memcpy(_data.data(), data, _data.size() * sizeof(T));
    }
    void clear(T color)
    {
        std::fill(_data.begin(), _data.end(), color);
    }
    GLuint bpp() const
    {
        return sizeof(T);
    }
    GLuint width() const
    {
        return _w;
    }
    GLuint height() const
    {
        return _h;
    }
    const T* row(GLuint r) const
    {
        return _data.data() + r * _w;
    }
    T* row(GLuint r)
    {
        return _data.data() + r * _w;
    }
};

// a tile inside a texture atlas
struct TextureTile
{
    GLint _x = 0, _y = 0;
    GLuint _w = 0, _h = 0, _parentW = 0, _parentH = 0;

    bool empty() const { return _w == 0 || _h == 0; }

    void translateCoordinate(float& u, float& v) const
    {
        float s = (u + _x) / _parentW;
        float t = (v + _y) / _parentH;
        u = s;
        v = t;
    }
};

template <Texture::Type TYPE>
struct TextureTypeTraits {};
template <>
struct TextureTypeTraits<Texture::RGBA>
{
    using PixelType = uint32_t;
};
template <>
struct TextureTypeTraits<Texture::GRAY>
{
    using PixelType = uint8_t;
};

template<Texture::Type TYPE>
class TextureAtlasBuilder
{
    using PixelType = typename TextureTypeTraits<TYPE>::PixelType;
    Image<PixelType> _textureImage;
    const GLuint _padding;
    
    enum { kMinRowHeight = 16 };
    struct Row
    {
        GLint _x, _y;
        GLuint _w, _h; // free space
        Row(GLint x, GLint y, GLuint w, GLuint h) :
        _x(x), _y(y), _w(w), _h(h)
        {}
    };
    std::vector<Row> _rows;
    GLuint _freeTop = 0;
public:
    TextureAtlasBuilder(GLuint size, GLuint padding) :
        _textureImage(size, size, nullptr), _padding(padding)
    {}

    TextureTile addImage(const Image<PixelType>& image)
    {
        const GLuint w = image.width();
        const GLuint h = image.height();
        const GLuint paddedW = w + _padding * 2;
        const GLuint paddedH = h + _padding * 2;

        //search for an allocated row that can fit
        auto i = _rows.begin();
        for (; i != _rows.end(); ++i)
        {
            if (i->_w >= paddedW && i->_h >= paddedH)
                break;
        }

        GLint paddedX, paddedY;

        if (i == _rows.end()) // no existing allocation fits this
        {
            GLuint top = _freeTop;
            GLuint allocH = std::max((GLuint)kMinRowHeight, paddedH);
            _freeTop += allocH;
            assert(_freeTop < _textureImage.height());
            // allocate a new row
            _rows.emplace_back(paddedW, top, _textureImage.width() - paddedW, allocH);
            paddedX = 0;
            paddedY = top;
        }
        else // found a row that can fit the image
        {
            paddedX = i->_x;
            paddedY = i->_y;
            i->_x += paddedW;
            i->_w -= paddedW;            
        }

        GLint x = paddedX + _padding;
        GLint y = paddedY + _padding;

        // now fill the subimage
        auto copyLinePadded = [] (auto* dst, const auto* src, int w, GLuint padding)
            {
                auto leftPadding = src[0];
                for (GLuint i = 0; i < padding; ++i)
                    *dst++ = leftPadding;
                for (GLuint i = 0; i < w; ++i)
                    *dst++ = *src++;
                auto rightPadding = src[-1];
                for (GLuint i = 0; i < padding; ++i)
                    *dst++ = rightPadding;
            };

        // top padding
        for (GLuint i = 0; i < _padding; ++i)
            copyLinePadded(_textureImage.row(paddedY + i) + paddedX, image.row(0), w, _padding);
        // image
        for (GLuint i = 0; i < h; ++i)
            copyLinePadded(_textureImage.row(y+i) + paddedX, image.row(i), w, _padding);
        // bottom padding
        for (GLuint i = 0; i < _padding; ++i)
            copyLinePadded(_textureImage.row(y + h + i) + paddedX, image.row(h-1), w, _padding);

        return {x, y, w, h, _textureImage.width(), _textureImage.height()};
    }

    std::unique_ptr<Texture> build()
    {
        auto texture = std::make_unique<Texture>(GL_TEXTURE_2D, _textureImage.width(), _textureImage.height(), TYPE, _textureImage.row(0));
        return std::move(texture);
    }
};
