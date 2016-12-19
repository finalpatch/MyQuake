#include <glbinding/Binding.h>

#include "glmodel.h"

extern "C"
{
#include "quakedef.h"

refdef_t r_refdef;
vec3_t	r_origin, vpn, vright, vup;
texture_s* r_notexture_mip = nullptr;
qboolean r_cache_thrash = qfalse;
}

std::unique_ptr<ModelRenderer> modelRenderer;

void R_Init (void)
{
    glbinding::Binding::initialize();    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void R_InitTextures (void)
{

}

void R_InitEfrags (void)
{

}

void R_RenderView (void)
{
    GLfloat w = 1280;
    GLfloat h = 720;

    static GLfloat bgColor[] = {0.27, 0.53, 0.71, 1.0};
    glViewport(0, 0, w, h);
    glClearBufferfv(GL_COLOR, 0, bgColor);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

    static int f = 0;
    modelRenderer->render(f++, 0, {}, {});
    if (f == 143 * 5)
        f = 0;
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
    auto playerModel = cl.model_precache[107];
    modelRenderer = std::make_unique<ModelRenderer>(playerModel);
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
