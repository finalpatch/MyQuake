#include <SDL2/SDL.h>
#include <glbinding/gl33core/gl.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

extern "C"
{
#include "quakedef.h"

refdef_t r_refdef;
vec3_t	r_origin, vpn, vright, vup;

texture_s *r_notexture_mip;
qboolean r_cache_thrash;
}

using namespace gl33core;

static std::string readFile(const std::string& filename)
{
    std::ifstream f(filename);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

class GLObject
{
protected:
    GLuint _handle;

public:
    virtual ~GLObject() {}

    GLuint handle() const
    {
        return _handle;
    }
};

class GLBuffer : public GLObject
{
public:
    GLBuffer(void* data, size_t size, size_t offset, BufferStorageMask flags)
    {
        glGenBuffers(1, &_handle);
        bind(GL_COPY_WRITE_BUFFER);
        glBufferStorage(GL_COPY_WRITE_BUFFER, size, data, flags);
    }
    ~GLBuffer() override
    {
        glDeleteBuffers(1, &_handle);
    }
    void Update(void* data, size_t size, size_t offset)
    {
        bind(GL_COPY_WRITE_BUFFER);
        glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data);
    }
    void bind(GLenum target)
    {
        glBindBuffer(target, _handle);
    }
    void bind(GLenum target, GLuint index)
    {
        glBindBufferBase(target, index, _handle);
    }
};

class Shader : public GLObject
{
public:
    Shader(GLenum type, const std::string& filename)
    {
        auto src = readFile(filename);
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
                Con_Printf("shader compilation failed\n");
                GLint errLength;
                glGetShaderiv(_handle, GL_INFO_LOG_LENGTH, &errLength);
                std::vector<char> errLog(errLength+1, '\0');
                glGetShaderInfoLog(_handle, errLength, &errLength, errLog.data());
                Con_Printf(errLog.data());
            }
        }
    }
    ~Shader() override
    {
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
    }
    ~RenderProgram() override
    {
        glDeleteProgram(_handle);
    }
    void use() const
    {
        glUseProgram(_handle);
    }
    void setUniformBlock(const std::string& name, GLBuffer& buf)
    {
        GLuint idx = glGetUniformBlockIndex(_handle, name.c_str());
        if (idx != GL_INVALID_INDEX)
            buf.bind(GL_UNIFORM_BUFFER, idx);
        else
            Con_Printf("invalid uniform name");
    }
};

class VertexArray : public GLObject
{
public:
    VertexArray()
    {
        glGenVertexArrays(1, &_handle);
        bind();
    }
    ~VertexArray() override
    {
        glDeleteVertexArrays(1, &_handle);
    }
    void enableAttrib(GLuint index)
    {
        glEnableVertexAttribArray(index);
    }
    void setVertexBuffer(GLBuffer& buf, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buf.handle());
        glVertexAttribPointer(index, size, type, normalized, stride, nullptr);
    }
    void setIndexBuffer(GLBuffer& buf)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.handle());
    }
    void bind()
    {
        glBindVertexArray(_handle);
    }
};

void R_Init (void)
{
}
void R_InitTextures (void)
{

}
void R_InitEfrags (void)
{

}
void R_RenderView (void)
{

}
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect)
{

}
void R_InitSky (struct texture_s *mt)
{

}

void R_AddEfrags (entity_t *ent)
{

}
void R_RemoveEfrags (entity_t *ent)
{

}

void R_NewMap (void)
{

}


void R_ParseParticleEffect (void)
{

}
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{

}
void R_RocketTrail (vec3_t start, vec3_t end, int type)
{

}

void R_EntityParticles (entity_t *ent)
{

}
void R_BlobExplosion (vec3_t org)
{

}
void R_ParticleExplosion (vec3_t org)
{

}
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
{

}
void R_LavaSplash (vec3_t org)
{

}
void R_TeleportSplash (vec3_t org)
{

}

void R_PushDlights (void)
{

}

void D_FlushCaches (void)
{

}
void D_InitCaches (void *buffer, int size)
{

}
void R_SetVrect (vrect_t *pvrect, vrect_t *pvrectin, int lineadj)
{

}
