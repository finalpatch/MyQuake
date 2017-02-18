#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// global palette
extern uint32_t vid_current_palette[256];

// camera matrices
struct Camera
{
    glm::vec3 eyePos;
    glm::vec3 eyeDir;
    glm::vec3 eyeUp;
    glm::mat4 projMat;
    glm::mat4 viewMat;
    glm::mat4 vpMat;

    Camera(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& up, float fov, float ar):
        eyePos(pos), eyeDir(dir), eyeUp(up)
    {
        projMat = glm::perspective(
            fov,      // y fov
            ar,       // aspect ratio
            1.0f,     // near
            5000.0f); // far
        viewMat = glm::lookAt(eyePos, eyePos + eyeDir, eyeUp);
        vpMat = projMat * viewMat;
    }
};

// particle interface to quake
void R_InitParticles ();
void R_ClearParticles();
void R_DrawParticles (const Camera& camera);

// particle interface to GL backend
struct particle_s;
void GL_StartParticles ();
void GL_DrawParticle (particle_s* pparticle);
void GL_EndParticles (const Camera& camera);

// setup for 2D
extern void R_BeginPictures();
