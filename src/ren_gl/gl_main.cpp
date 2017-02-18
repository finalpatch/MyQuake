#include <glbinding/Binding.h>
#include <unordered_map>

#include "gl_model.h"
#include "gl_level.h"
#include "gl_local.h"

extern "C"
{
#include "quakedef.h"

refdef_t r_refdef;
vec3_t	r_origin, vpn, vright, vup;
texture_s* r_notexture_mip = nullptr;
qboolean r_cache_thrash = qfalse;
}

extern cvar_t scr_fov;

/*
 TODO:
 * Post processing
 */

std::unique_ptr<LevelRenderer> levelRenderer;
std::vector<std::unique_ptr<ModelRenderer>> modelRenderers;

void drawLevel(const Camera& camera);
void drawEntities(const Camera& camera);
void drawWeapon(const Camera& camera);

void R_Init (void)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    glEnable(GL_PROGRAM_POINT_SIZE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    R_InitParticles();
}

void R_InitTextures (void)
{

}

void R_RenderView (void)
{
    // sound output uses these
   	VectorCopy(r_refdef.vieworg, r_origin);
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

    // get ready for 3D
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // camera
    Camera camera(qvec2glm(r_origin), qvec2glm(vpn), qvec2glm(vup),
        glm::radians(scr_fov.value * 0.75f), (float)vid.width / vid.height);

    // clear buffers
    static GLfloat bgColor[] = {0.27, 0.53, 0.71, 1.0};
    glViewport(0, 0, vid.width, vid.height);
    glClearBufferfv(GL_COLOR, 0, bgColor);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

    drawLevel(camera);
    drawEntities(camera);
    drawWeapon(camera);
    R_DrawParticles(camera);

    // get ready for 2D
    R_BeginPictures();
}

void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect)
{
}

void R_InitSky (struct texture_s *mt)
{

}

void R_NewMap (void)
{
    Timing timing;
    modelRenderers.clear();
    levelRenderer = std::make_unique<LevelRenderer>();

    for (int i = 0; i < MAX_MODELS; ++i)
    {
        model_s* model = cl.model_precache[i];
        if (model == nullptr)
            continue;
        if (model->type == mod_brush)
        {
            Sys_Printf("load brush model %d: %s\n", i, model->name);
            levelRenderer->loadBrushModel(model);
        }
        else if (model->type == mod_alias)
        {
            Sys_Printf("load alias model %d: %s\n", i, model->name);
            model->rendererData = modelRenderers.size();
            modelRenderers.emplace_back(std::make_unique<ModelRenderer>(model));
        }
    }

    levelRenderer->build();
    R_ClearParticles();
    timing.print("load level");
}

struct ParticleVertex
{
    GLfloat position[3];
    GLuint  color;
};
std::vector<ParticleVertex> activeParticles;
std::unique_ptr<VertexArray> particleVao;

void GL_StartParticles ()
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

void GL_DrawParticle (particle_s* pparticle)
{
    activeParticles.emplace_back(ParticleVertex{{
            pparticle->org[0], pparticle->org[2], -pparticle->org[1]
        },
        vid_current_palette[uint8_t(pparticle->color)]});
}

void GL_EndParticles (const Camera& camera)
{
    if (activeParticles.empty())
        return;
    GLBuffer<ParticleVertex, GL_ARRAY_BUFFER> particleBuffer(activeParticles.data(), activeParticles.size());
    particleVao->bind();
    particleVao->vertexBuffer(ParticleRenderPass::kVertexInputVertex, particleBuffer, 0);
    particleVao->vertexBuffer(ParticleRenderPass::kVertexInputColor, particleBuffer, offsetof(ParticleVertex, color));

    glm::vec3 eyePos = qvec2glm(r_origin);
    ParticleRenderPass::getInstance().use();
    ParticleRenderPass::getInstance().setup(camera.projMat, camera.viewMat, glm::vec4(eyePos, 1.0));
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

void drawLevel(const Camera& camera)
{
    levelRenderer->renderWorld(camera, &cl_entities[0]);
}

void drawEntities(const Camera& camera)
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
                    levelRenderer->renderSubmodel(camera, currentEntity);
            }
            break;
		case mod_sprite:
            break;
		case mod_alias:
            {
                auto model = currentEntity->model;
                auto& modelRenderer = modelRenderers[model->rendererData];
                float ambientLight = levelRenderer->lightPoint(currentEntity->origin);
                modelRenderer->render(camera, currentEntity, ambientLight);
            }
            break;
        default:
            break;
        }
    }
}

void drawWeapon(const Camera& camera)
{
    auto entity = &cl.viewent;
    auto model = entity->model;
    if (!model)
        return;
    auto& renderer = modelRenderers[model->rendererData];
    // allways give some light on gun
    float ambientLight = std::max(0.1f, levelRenderer->lightPoint(entity->origin));
    glDepthFunc(GL_ALWAYS);
    renderer->render(camera, entity, ambientLight);
    glDepthFunc(GL_LESS);
    renderer->render(camera, entity, ambientLight);
}
