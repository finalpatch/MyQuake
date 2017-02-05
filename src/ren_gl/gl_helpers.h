#pragma once

#include <array>
#include <algorithm>
#include <memory>
#include "gl_utils.h"

#ifdef GL45
    #include <glbinding/gl45core/gl.h>
    using namespace gl45core;
#else
    #include <glbinding/gl33core/gl.h>
    #include <glbinding/gl/extension.h> 
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
    GLGenericBuffer()
    {}
    GLGenericBuffer(GLGenericBuffer&& other) : GLObject(std::move(other))
    {}
    virtual void bind(GLenum target)
    {
        glBindBuffer(target, _handle);
    }
    virtual void bind(GLenum target, GLuint index)
    {
        glBindBufferBase(target, index, _handle);
    }
    virtual size_t size() const = 0;
    virtual size_t stride() const = 0;
};

template<typename T, GLenum BUFFER_TYPE>
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
    explicit GLBuffer(const std::vector<T>& data, BufferStorageMask flags = GL_NONE_BIT)
        : GLBuffer(data.data(), data.size(), flags)
    {}
#else
    GLBuffer(const T* data, size_t size, GLenum flags = GL_STATIC_DRAW)
        : _size(size)
    {
        glGenBuffers(1, &_handle);
        bind(BUFFER_TYPE);
        glBufferData(BUFFER_TYPE, size * sizeof(T), data, flags);
    }
    explicit GLBuffer(const std::vector<T>& data, GLenum flags = GL_STATIC_DRAW)
        : GLBuffer(data.data(), data.size(), flags)
    {}
#endif
    GLBuffer(GLBuffer&& other) : GLGenericBuffer(std::move(other))
    {
        _size = other.size();
    }
    ~GLBuffer() override
    {
        if (_handle)
            glDeleteBuffers(1, &_handle);
    }
    size_t size() const override
    {
        return _size;
    }
    size_t stride() const override
    {
        return sizeof(T);
    }
    void update(const T* data, size_t size, size_t offset = 0)
    {
#ifdef GL45
        glNamedBufferSubData(_handle, offset * sizeof(T), size * sizeof(T), data);
#else
        bind(BUFFER_TYPE);
        glBufferSubData(BUFFER_TYPE, offset * sizeof(T), size * sizeof(T), data);
#endif
    }
    void update(const std::vector<T>& data, size_t offset = 0)
    {
        update(data.data(), data.size(), offset);
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
                printf("%s", errLog.data());
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
    GLuint getUniformBlockIndex(const char* name) const
    {
        auto idx = glGetUniformBlockIndex(_handle, name);
        assert(idx != GL_INVALID_INDEX);
        return idx;
    }
    void setUniformBlock(const char* name, GLGenericBuffer& buf)
    {
        setUniformBlock(getUniformBlockIndex(name), buf);
    }
    void setUniformBlock(GLuint idx, GLGenericBuffer& buf)
    {
        buf.bind(GL_UNIFORM_BUFFER, idx);
    }
    GLuint getUniformLocation(const char* name) const
    {
        return glGetUniformLocation(_handle, name);
    }
    void assignTextureUnit(const char* textureName, GLint textureUnit)
    {
        assignTextureUnit(getUniformLocation(textureName), textureUnit);
    }
    void assignTextureUnit(GLuint loc, GLint textureUnit)
    {
        glUniform1i(loc, textureUnit);
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
        GLboolean _isInt = GL_FALSE;
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
        GLObject(std::move(other))
#ifndef GL45
        ,_formats(std::move(other._formats))
#endif
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
    void format(GLuint index, GLint size, GLenum type, GLboolean normalized, GLboolean isInt = GL_FALSE)
    {
#ifdef GL45
        if (!isInt)
            glVertexArrayAttribFormat(_handle, index, size, type, normalized, 0);
        else
            glVertexArrayAttribIFormat(_handle, index, size, type, 0);
#else
        if (_formats.size() <= index)
            _formats.resize(index + 1);
        _formats[index]._size = size;
        _formats[index]._type = type;
        _formats[index]._normalized = normalized;
        _formats[index]._isInt = isInt;
#endif
    }
    void vertexBuffer(GLuint index, GLGenericBuffer& buf, GLintptr offset = 0)
    {
#ifdef GL45
        glVertexArrayAttribBinding(_handle, index, index);
        glVertexArrayVertexBuffer(_handle, index, buf.handle(), offset, buf.stride());
#else
        if (_formats.size() <= index)
            _formats.resize(index + 1);
        _formats[index]._stride = buf.stride();
        _formats[index]._offset = offset;
        
        buf.bind(GL_ARRAY_BUFFER);
        if (!_formats[index]._isInt)
            glVertexAttribPointer(index, _formats[index]._size, _formats[index]._type,
                _formats[index]._normalized, _formats[index]._stride,
                (const void*)_formats[index]._offset);
        else
            glVertexAttribIPointer(index, _formats[index]._size, _formats[index]._type,
                _formats[index]._stride, (const void*)_formats[index]._offset);
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
    Texture(GLenum target, GLuint width, GLuint height, Type type,
        GLenum wrap, GLenum minfilter, GLenum magfilter,
        const GLvoid* data = nullptr)
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
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);

        float aniso = 0.0f;
        glGetFloatv(gl::GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
        glTexParameterf(GL_TEXTURE_2D, gl::GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(8.0f, aniso));
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
    void setMaxMipLevel(GLuint level)
    {
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level);
    }
    void addMipmap(GLuint w, GLuint h, const GLvoid* data, GLuint level)
    {
        glBindTexture(_target, _handle);
        glTexImage2D(_target, level, _internalFormat, w, h, 0, _format, _datatype, data);
    }
    void update(GLint x, GLint y, GLuint w, GLuint h, const GLvoid* data, GLuint level = 0)
    {
        glBindTexture(_target, _handle);
        glTexSubImage2D(_target, level, x, y, w, h, _format, _datatype, data);
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
    TextureBinding(const Texture& texture, GLuint textureUnit = 0) :
        _boundUnit(static_cast<GLenum>(textureUnit + static_cast<GLuint>(GL_TEXTURE0))),
        _texture(texture)
    {
        glActiveTexture(_boundUnit);
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
            std::copy_n(data, _data.size(), _data.data());
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

    std::unique_ptr<Texture> buildTexture(GLenum wrap, GLenum minfilter, GLenum magfilter)
    {
        auto texture = std::make_unique<Texture>(GL_TEXTURE_2D, _textureImage.width(), _textureImage.height(), TYPE,
            wrap, minfilter, magfilter, _textureImage.row(0));
        return std::move(texture);
    }
};
