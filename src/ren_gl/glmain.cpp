#include <glbinding/Binding.h>

#include <unordered_map>

#include "glmodel.h"
#include "gllevel.h"
#include "glrenderpass.h"

extern "C"
{
#include "quakedef.h"

refdef_t r_refdef;
vec3_t	r_origin, vpn, vright, vup;
texture_s* r_notexture_mip = nullptr;
qboolean r_cache_thrash = qfalse;
}

/*
 TODO:
 * Fullbrights
 * Texture animation
 * Procedural textures
 * Particals
 * Dynamic lighting
 */

std::unique_ptr<LevelRenderer> levelRenderer;
std::vector<std::unique_ptr<ModelRenderer>> modelRenderers;

void drawLevel();
void drawEntities();
void drawWeapon();

void R_Init (void)
{
    glbinding::Binding::initialize();
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
}

void R_InitTextures (void)
{

}

void R_RenderView (void)
{
    // sound output uses these
   	VectorCopy(r_refdef.vieworg, r_origin);
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

    static GLfloat bgColor[] = {0.27, 0.53, 0.71, 1.0};
    glViewport(0, 0, vid.width, vid.height);
    glClearBufferfv(GL_COLOR, 0, bgColor);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

    DefaultRenderPass::getInstance().use();
    drawLevel();
    drawEntities();
    drawWeapon();
}

void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect)
{

}

void R_InitSky (struct texture_s *mt)
{

}

void R_NewMap (void)
{
    modelRenderers.clear();
    levelRenderer = std::make_unique<LevelRenderer>();

    for (int i = 0; i < MAX_MODELS; ++i)
    {
        model_s* model = cl.model_precache[i];
        if (model == nullptr)
            continue;
        if (model->type == mod_brush)
        {
            Con_Printf("load brush model %d: %s\n", i, model->name);
            levelRenderer->loadBrushModel(model);
        }
        else if (model->type == mod_alias)
        {
            Con_Printf("load alias model %d: %s\n", i, model->name);
            model->rendererData = modelRenderers.size();
            modelRenderers.emplace_back(std::make_unique<ModelRenderer>(model));
        }
    }

    levelRenderer->build();
}

void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0/16);
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

    if (msgcount == 255)
        count = 1024;
    else
        count = msgcount;
	
	R_RunParticleEffect (org, dir, color, count);
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

// ************************************************************************

void drawLevel()
{
    levelRenderer->renderWorld();
}

void drawEntities()
{
    for (int i = 0; i < cl_numvisedicts; ++i)
    {
        auto currentEntity = cl_visedicts[i];
        if (currentEntity == &cl_entities[cl.viewentity]) // player
            continue;
        if (!currentEntity->model)
            continue;
		switch (currentEntity->model->type)
		{
        case mod_brush:
            {
                auto model = currentEntity->model;
                if (model == cl.worldmodel)
                    break;
                levelRenderer->renderSubmodel(
                        model,
                        currentEntity->origin,
                        currentEntity->angles);
            }
            break;
		case mod_sprite:
            break;
		case mod_alias:
            {
                auto model = currentEntity->model;
                auto& modelRenderer = modelRenderers[model->rendererData];
                modelRenderer->render(
                    currentEntity->frame,
                    cl.time + currentEntity->syncbase,
                    currentEntity->origin,
                    currentEntity->angles,
                    levelRenderer->lightPoint(currentEntity->origin));
            }
            break;
        default:
            break;
        }
    }
}

void drawWeapon()
{
    auto entity = &cl.viewent;
    auto model = entity->model;
    if (!model)
        return;
    auto& renderer = modelRenderers[model->rendererData];
    // allways give some light on gun
    float ambientLight = std::max(0.1f, levelRenderer->lightPoint(entity->origin));
    renderer->render(entity->frame, cl.time + entity->syncbase,
        entity->origin, entity->angles, ambientLight);
}
