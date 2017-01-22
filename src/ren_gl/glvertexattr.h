#pragma once

#include "glhelper.h"

struct VertexAttr
{
    GLvec3 vertex;
    GLvec3 normal;
    GLvec2 lightuv;
    GLvec2 diffuseuv;
    GLubyte styles[4];
    GLuint vtxflags;
};
