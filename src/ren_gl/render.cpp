extern "C"
{
#include "quakedef.h"

refdef_t r_refdef;
vec3_t	r_origin, vpn, vright, vup;

texture_s *r_notexture_mip;
qboolean r_cache_thrash;
}

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
