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

void R_InitParticles ();
void R_ClearParticles();
void R_DrawParticles ();
}

extern uint32_t vid_current_palette[256];

/*
 TODO:
 * refactor texture loading
 * Fullbrights
 * Post processing
 * Weapon clipping
 */

std::unique_ptr<LevelRenderer> levelRenderer;
std::vector<std::unique_ptr<ModelRenderer>> modelRenderers;

extern cvar_t	scr_fov;
glm::mat4 r_projection;

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

    glEnable(GL_PROGRAM_POINT_SIZE);

    R_InitParticles();
}

void R_InitTextures (void)
{

}

void R_RenderView (void)
{
    r_projection = glm::perspective(
        glm::radians(scr_fov.value * 0.75f), // y fov
        (float)vid.width / vid.height,       // aspect ratio
        1.0f,                                // near
        5000.0f);                            // far

    // sound output uses these
   	VectorCopy(r_refdef.vieworg, r_origin);
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

    static GLfloat bgColor[] = {0.27, 0.53, 0.71, 1.0};
    glViewport(0, 0, vid.width, vid.height);
    glClearBufferfv(GL_COLOR, 0, bgColor);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

    drawLevel();
    drawEntities();
    drawWeapon();

    R_DrawParticles();
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
    R_ClearParticles();
}

struct ParticleVertex
{
    GLfloat position[3];
    GLuint  color;
};
std::vector<ParticleVertex> activeParticles;
std::unique_ptr<VertexArray> particleVao;

void D_StartParticles (void)
{
    activeParticles.clear();

    if (!particleVao)
    {
        particleVao = std::make_unique<VertexArray>();
        particleVao->enableAttrib(0);
        particleVao->format(0, 3, GL_FLOAT, GL_FALSE);
        particleVao->enableAttrib(1);
        particleVao->format(1, 4, GL_UNSIGNED_BYTE, GL_TRUE);
    }
}

void D_DrawParticle (particle_t *pparticle)
{
    activeParticles.emplace_back(ParticleVertex{{
            pparticle->org[0], pparticle->org[2], -pparticle->org[1]
        },
        vid_current_palette[uint8_t(pparticle->color)]});
}

void D_EndParticles (void)
{
    if (activeParticles.empty())
        return;
    GLBuffer<ParticleVertex> particleBuffer(activeParticles.data(), activeParticles.size());
    particleVao->bind();
    particleVao->vertexBuffer(ParticleRenderPass::kVertexInputVertex, particleBuffer, 0);
    particleVao->vertexBuffer(ParticleRenderPass::kVertexInputColor, particleBuffer, offsetof(ParticleVertex, color));

    glm::vec3 eyePos = qvec2glm(r_origin);
    glm::vec3 eyeDirection = qvec2glm(vpn);
    glm::mat4 viewMatrix = glm::lookAt(eyePos, eyePos + eyeDirection, qvec2glm(vup));
    ParticleRenderPass::getInstance().use();
    ParticleRenderPass::getInstance().setup(r_projection, viewMatrix, glm::vec4(eyePos, 1.0));
    glDrawArrays(GL_POINTS, 0, particleBuffer.size());
}

void R_PushDlights (void)
{
    std::vector<GLfloat> dlightBuf;
    for (int i = 0 ; i < MAX_DLIGHTS; ++i)
    {
        if (cl_dlights[i].die <= cl.time || !cl_dlights[i].radius)
            continue;
        dlightBuf.insert(dlightBuf.end(), {
            cl_dlights[i].origin[0],
            cl_dlights[i].origin[2],
            -cl_dlights[i].origin[1],
            cl_dlights[i].radius});
    }
    DefaultRenderPass::getInstance().updateDLights(dlightBuf.data(), dlightBuf.size()/4);
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
    levelRenderer->renderWorld(&cl_entities[0]);
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
                    break;//levelRenderer->renderWorld(currentEntity);
                else
                    levelRenderer->renderSubmodel(currentEntity);
            }
            break;
		case mod_sprite:
            break;
		case mod_alias:
            {
                auto model = currentEntity->model;
                auto& modelRenderer = modelRenderers[model->rendererData];
                float ambientLight = levelRenderer->lightPoint(currentEntity->origin);
                modelRenderer->render(
                    currentEntity->frame,
                    cl.time + currentEntity->syncbase,
                    currentEntity->origin,
                    currentEntity->angles,
                    ambientLight);
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
