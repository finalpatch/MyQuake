#include "gllevel.h"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern "C"
{
#include "quakedef.h"
}

enum {
    kVertexInputVertex = 0,
    kVertexInputNormal = 1
};

class LevelRenderProgram
{
    struct UniformBlock
    {
        GLfloat model[4*4];
        GLfloat view[4*4];
        GLfloat projection[4*4];
    };
public:
    static void setup(float w, float h, const glm::mat4& model, const glm::mat4& view)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 5000.0f);
        UniformBlock uniformBlock;
        memcpy(uniformBlock.model, glm::value_ptr(model), sizeof(uniformBlock.model));
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        getInstance()._ufmBuf->update(&uniformBlock);
    }
    static void use()
    {
        getInstance()._prog->use();
        getInstance()._prog->setUniformBlock("TransformBlock", * getInstance()._ufmBuf);
    }

private:
    LevelRenderProgram()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);
        _ufmBuf = std::make_unique<GLBuffer<UniformBlock>>(nullptr, 1, GL_DYNAMIC_STORAGE_BIT);
    }

    static LevelRenderProgram& getInstance()
    {
        static LevelRenderProgram singleton;
        return singleton;
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer<UniformBlock>> _ufmBuf;
};

LevelRenderer::LevelRenderer(const model_s* levelModel)
{
    std::vector<GLvec3> vertexBuffer;
    std::vector<GLvec3> normalBuffer;

    int triangles = 0;
    for (int i = 0; i < levelModel->numsurfaces; ++i)
    {
        const auto& surface = levelModel->surfaces[i];

        if (strcmp(surface.texinfo->texture->name, "trigger") == 0)
            continue;

        int firstVertexIndex;
        int previousVertexIndex;
        
        for (int j = 0; j < surface.numedges; ++j)
        {
            auto edgeIndex = levelModel->surfedges[surface.firstedge + j];
            int currentVertexIndex;

            if (edgeIndex > 0)
            {
                auto& edge = levelModel->edges[edgeIndex];
                currentVertexIndex = edge.v[0];
            }
            else
            {
                auto& edge = levelModel->edges[-edgeIndex];
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
                glm::vec3 v0 = qvec2glm(levelModel->vertexes[firstVertexIndex].position);
                glm::vec3 v1 = qvec2glm(levelModel->vertexes[previousVertexIndex].position);
                glm::vec3 v2 = qvec2glm(levelModel->vertexes[currentVertexIndex].position);

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

void LevelRenderer::render()
{
	auto viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);
    _oldviewleaf = viewleaf;

    glm::vec3 origin = qvec2glm(cl_visedicts[0]->origin);

    glm::vec3 eyePos = qvec2glm(r_origin);
    glm::vec3 eyeDirection = qvec2glm(vpn);
    glm::mat4 model;
    glm::mat4 view = glm::lookAt(eyePos, eyePos + eyeDirection, qvec2glm(vup));

    LevelRenderProgram::setup(vid.width, vid.height, model, view);
    LevelRenderProgram::use();
    _vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, _vtxBuf->size());
}

void LevelRenderer::markLeaves (mleaf_s* viewleaf)
{
	byte	*vis;
	mnode_t	*node;
	int		i;

	if (_oldviewleaf == viewleaf)
		return;

	_visframecount++;
	_oldviewleaf = viewleaf;

	vis = Mod_LeafPVS (viewleaf, cl.worldmodel);

	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == _visframecount)
					break;
				node->visframe = _visframecount;
				node = node->parent;
			} while (node);
		}
	}
}
