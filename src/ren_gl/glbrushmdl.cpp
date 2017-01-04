#include "glbrushmdl.h"
#include "glrenderprg.h"

#include <algorithm>

extern "C"
{
#include "quakedef.h"
}

BrushModelRenderer::BrushModelRenderer(const model_s* brushModel)
{
    std::vector<GLvec3> vertexBuffer;
    std::vector<GLvec3> normalBuffer;

    for (int i = 0; i < brushModel->nummodelsurfaces; ++i)
    {
        auto& surface = brushModel->surfaces[brushModel->firstmodelsurface + i];

        int firstVertexIndex;
        int previousVertexIndex;

        for (int j = 0; j < surface.numedges; ++j)
        {
            auto edgeIndex = brushModel->surfedges[surface.firstedge + j];
            int currentVertexIndex;

            if (edgeIndex > 0)
            {
                auto& edge = brushModel->edges[edgeIndex];
                currentVertexIndex = edge.v[0];
            }
            else
            {
                auto& edge = brushModel->edges[-edgeIndex];
                currentVertexIndex = edge.v[1];
            }

            if (j == 0)
            {
                firstVertexIndex = currentVertexIndex;
            }
            else if (j == 1)
            {
                previousVertexIndex = currentVertexIndex;
            }
            else
            {
                glm::vec3 v0 = qvec2glm(brushModel->vertexes[firstVertexIndex].position);
                glm::vec3 v1 = qvec2glm(brushModel->vertexes[previousVertexIndex].position);
                glm::vec3 v2 = qvec2glm(brushModel->vertexes[currentVertexIndex].position);

                vertexBuffer.emplace_back(GLvec3{v0[0], v0[1], v0[2]});
                vertexBuffer.emplace_back(GLvec3{v1[0], v1[1], v1[2]});
                vertexBuffer.emplace_back(GLvec3{v2[0], v2[1], v2[2]});

                glm::vec3 n = glm::normalize(qvec2glm(surface.plane->normal));
                if (surface.flags & SURF_PLANEBACK)
                    n *= -1;

                normalBuffer.emplace_back(GLvec3{n[0], n[1], n[2]});
                normalBuffer.emplace_back(GLvec3{n[0], n[1], n[2]});
                normalBuffer.emplace_back(GLvec3{n[0], n[1], n[2]});

                previousVertexIndex = currentVertexIndex;
            }
        }
    }
    _vtxBuf = std::make_unique<GLBuffer<GLvec3>>(vertexBuffer.data(), vertexBuffer.size());
    _nrmBuf = std::make_unique<GLBuffer<GLvec3>>(normalBuffer.data(), normalBuffer.size());
    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(kVertexInputVertex);
    _vao->format(kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputVertex, *_vtxBuf, sizeof(GLvec3));

    _vao->enableAttrib(kVertexInputNormal);
    _vao->format(kVertexInputNormal, 3, GL_FLOAT, GL_TRUE);
    _vao->vertexBuffer(kVertexInputNormal, *_nrmBuf, sizeof(GLvec3));
}

BrushModelRenderer::~BrushModelRenderer()
{
}

void BrushModelRenderer::render(const float* origin, const float* angles)
{
    glm::vec3 pos = qvec2glm(origin);
    glm::vec3 eyePos = qvec2glm(r_origin);
    glm::vec3 eyeDirection = qvec2glm(vpn);
    glm::mat4 model = 
        glm::translate(glm::mat4(), pos)
        * glm::rotate(glm::mat4(), glm::radians(angles[1]), {0, 1, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[0]), {1, 0, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[2]), {0, 0, 1});
    glm::mat4 view = glm::lookAt(eyePos, eyePos + eyeDirection, qvec2glm(vup));

    QuakeRenderProgram::setup(vid.width, vid.height, model, view);
    _vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, _vtxBuf->size());
}
