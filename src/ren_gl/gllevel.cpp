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
    static void use(float w, float h, const glm::mat4& model, const glm::mat4& view)
    {
        auto projection = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 5000.0f);
        UniformBlock uniformBlock;
        memcpy(uniformBlock.model, glm::value_ptr(model), sizeof(uniformBlock.model));
        memcpy(uniformBlock.view, glm::value_ptr(view), sizeof(uniformBlock.view));
        memcpy(uniformBlock.projection, glm::value_ptr(projection), sizeof(uniformBlock.projection));
        getInstance()._ufmBuf->update(&uniformBlock);
        getInstance()._prog->use();
    }

private:
    LevelRenderProgram()
    {
        std::vector<Shader> shaders;
        shaders.emplace_back(GL_VERTEX_SHADER, readTextFile("shaders/vs.glsl"));
        shaders.emplace_back(GL_FRAGMENT_SHADER, readTextFile("shaders/ps.glsl"));
        _prog = std::make_unique<RenderProgram>(shaders);

        _ufmBuf = std::make_unique<GLBuffer>(nullptr, sizeof(UniformBlock), GL_DYNAMIC_STORAGE_BIT);
        _prog->setUniformBlock("TransformBlock", *_ufmBuf);
    }

    static LevelRenderProgram& getInstance()
    {
        static LevelRenderProgram singleton;
        return singleton;
    }

    std::unique_ptr<RenderProgram> _prog;
    std::unique_ptr<GLBuffer>      _ufmBuf;
};

LevelRenderer::LevelRenderer(const model_s* levelModel)
{
    std::vector<GLfloat> vertexBuffer;
    std::vector<GLfloat> normalBuffer;

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

                vertexBuffer.push_back(v0[0]); vertexBuffer.push_back(v0[1]); vertexBuffer.push_back(v0[2]);
                vertexBuffer.push_back(v1[0]); vertexBuffer.push_back(v1[1]); vertexBuffer.push_back(v1[2]);
                vertexBuffer.push_back(v2[0]); vertexBuffer.push_back(v2[1]); vertexBuffer.push_back(v2[2]);

                glm::vec3 normal = qvec2glm(surface.plane->normal);
                normalBuffer.push_back(normal[0]); normalBuffer.push_back(normal[1]); normalBuffer.push_back(normal[2]);
                normalBuffer.push_back(normal[0]); normalBuffer.push_back(normal[1]); normalBuffer.push_back(normal[2]);
                normalBuffer.push_back(normal[0]); normalBuffer.push_back(normal[1]); normalBuffer.push_back(normal[2]);

                previousVertexIndex = currentVertexIndex;
            }
        }
    }

    _vtxBuf = std::make_unique<GLBuffer>(vertexBuffer.data(), vertexBuffer.size() * sizeof(GLfloat));
    _nrmBuf = std::make_unique<GLBuffer>(normalBuffer.data(), normalBuffer.size() * sizeof(GLfloat));

    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(kVertexInputVertex);
    _vao->format(kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputVertex, *_vtxBuf, 3 * sizeof(GLfloat));

    _vao->enableAttrib(kVertexInputNormal);
    _vao->format(kVertexInputNormal, 3, GL_FLOAT, GL_TRUE);
    _vao->vertexBuffer(kVertexInputNormal, *_nrmBuf, 3 * sizeof(GLfloat));
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

    LevelRenderProgram::use(vid.width, vid.height, model, view);
    _vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, _vtxBuf->size() / (sizeof(GLfloat) * 3));
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
