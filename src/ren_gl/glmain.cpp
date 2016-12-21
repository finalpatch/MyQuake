#include <glbinding/Binding.h>

#include <unordered_map>

#include "glmodel.h"

extern "C"
{
#include "quakedef.h"

refdef_t r_refdef;
vec3_t	r_origin, vpn, vright, vup;
texture_s* r_notexture_mip = nullptr;
qboolean r_cache_thrash = qfalse;
}

std::unordered_map<model_t*, std::unique_ptr<ModelRenderer>> modelRenderers;

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
    static GLfloat bgColor[] = {0.27, 0.53, 0.71, 1.0};
    glViewport(0, 0, vid.width, vid.height);
    glClearBufferfv(GL_COLOR, 0, bgColor);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

    for (int i = 0; i < cl_numvisedicts; ++i)
    {
        auto currentEntity = cl_visedicts[i];
        if (currentEntity == &cl_entities[cl.viewentity])
            continue;
		switch (currentEntity->model->type)
		{
		case mod_sprite:
            break;
		case mod_alias:
            {
                auto model = currentEntity->model;
                auto& modelRenderer = modelRenderers[model];
                if (modelRenderer)
                {
                    modelRenderer->render(
                        currentEntity->frame,
                        cl.time + currentEntity->syncbase,
                        currentEntity->origin,
                        currentEntity->angles);
                }
            }
            break;
        default:
            break;
        }
    }

    // static int f = 0;
    // modelRenderers[cl.model_precache[133]]->render(0, cl.time, {}, {});
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
    modelRenderers.clear();

    for (int i = 0; i < MAX_MODELS; ++i)
    {
        auto model = cl.model_precache[i];
        if (model != nullptr && model->type == mod_alias)
            modelRenderers.emplace(model, std::make_unique<ModelRenderer>(model));
    }
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
