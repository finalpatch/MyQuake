#include "gllevel.h"
#include "glrenderprg.h"

#include <algorithm>

extern "C"
{
#include "quakedef.h"
}

const static GLuint kLightmapAtlasSize = 2048;
const static GLuint kLightmapAtlasPadding = 1;

LevelRenderer::LevelRenderer(const model_s* levelModel)
{
    std::vector<GLvec3> vertexBuffer;
    std::vector<GLvec3> normalBuffer;
    
    TextureAtlasBuilder<Texture::GRAY> lightmapBuilder(kLightmapAtlasSize, kLightmapAtlasPadding);

    for (int i = 0; i < levelModel->numsurfaces; ++i)
    {
        auto& surface = levelModel->surfaces[i];

        surface.rendererInfo = _rendererInfoArray.size();        
        RendererInfo surfaceRendererInfo;
        surfaceRendererInfo.vertexIndex = vertexBuffer.size();
        _rendererInfoArray.push_back(surfaceRendererInfo);

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
    _idxBuf = std::make_unique<GLBuffer<GLuint>>(nullptr, vertexBuffer.size(), kGlBufferDynamic);

    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(kVertexInputVertex);
    _vao->format(kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputVertex, *_vtxBuf, sizeof(GLvec3));

    _vao->enableAttrib(kVertexInputNormal);
    _vao->format(kVertexInputNormal, 3, GL_FLOAT, GL_TRUE);
    _vao->vertexBuffer(kVertexInputNormal, *_nrmBuf, sizeof(GLvec3));
    _vao->indexBuffer(*_idxBuf);

    _lightmap = lightmapBuilder.build(GL_TEXTURE_2D);
}

void LevelRenderer::render()
{
    _framecount++;
	auto viewleaf = Mod_PointInLeaf(r_origin, cl.worldmodel);
    markLeaves(viewleaf);

    std::vector<GLuint> indexBuffer;
    walkBspTree(cl.worldmodel->nodes, indexBuffer);
    _idxBuf->update(indexBuffer.data(), indexBuffer.size());

    glm::vec3 origin = qvec2glm(cl_visedicts[0]->origin);
    glm::vec3 eyePos = qvec2glm(r_origin);
    glm::vec3 eyeDirection = qvec2glm(vpn);
    glm::mat4 model;
    glm::mat4 view = glm::lookAt(eyePos, eyePos + eyeDirection, qvec2glm(vup));

    QuakeRenderProgram::setup(vid.width, vid.height, model, view);
    _vao->bind();
    glDrawElements(GL_TRIANGLES, indexBuffer.size(), GL_UNSIGNED_INT, nullptr);
}

void LevelRenderer::markLeaves (mleaf_s* viewleaf)
{
	if (_oldviewleaf == viewleaf)
		return;

	_visframecount++;
	_oldviewleaf = viewleaf;

	auto vis = Mod_LeafPVS (viewleaf, cl.worldmodel);

	for (int i=0 ; i<cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			auto node = (mnode_t *)&cl.worldmodel->leafs[i+1];
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

void LevelRenderer::storeEfrags (efrag_s **ppefrag)
{
	entity_t	*pent;
	model_t		*clmodel;
	efrag_t		*pefrag;

	while ((pefrag = *ppefrag) != NULL)
	{
		pent = pefrag->entity;
		clmodel = pent->model;

		switch (clmodel->type)
		{
		case mod_alias:
		case mod_brush:
		case mod_sprite:
			pent = pefrag->entity;

			if ((pent->visframe != _framecount) &&
				(cl_numvisedicts < MAX_VISEDICTS))
			{
				cl_visedicts[cl_numvisedicts++] = pent;

			// mark that we've recorded this entity for this frame
				pent->visframe = _framecount;
			}

			ppefrag = &pefrag->leafnext;
			break;

		default:	
			Sys_Error ("R_StoreEfrags: Bad entity type %d\n", clmodel->type);
		}
	}
}

void LevelRenderer::walkBspTree(mnode_s *node, std::vector<GLuint>& indexBuffer)
{
	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != _visframecount)
		return;

    if (node->contents < 0) // leaf node
    {
		auto pleaf = (mleaf_t *)node;
        for (int i = 0; i < pleaf->nummarksurfaces; ++i)
            pleaf->firstmarksurface[i]->visframe = _visframecount;
        
        if (pleaf->efrags)
            storeEfrags(&pleaf->efrags);
    }
    else
    {
        auto plane = node->plane;
        double dot;
		switch (plane->type)
		{
		case PLANE_X:
			dot = r_origin[0] - plane->dist;
			break;
		case PLANE_Y:
			dot = r_origin[1] - plane->dist;
			break;
		case PLANE_Z:
			dot = r_origin[2] - plane->dist;
			break;
		default:
			dot = DotProduct (r_origin, plane->normal) - plane->dist;
			break;
		}	
        int side = (dot >= 0) ? 0 : 1;

        // visit near
        walkBspTree(node->children[side], indexBuffer);

        // emit marked polygons
        auto surf = cl.worldmodel->surfaces + node->firstsurface;
        for (int i = 0; i < node->numsurfaces; ++i)
        {
            if(surf[i].visframe != _visframecount)
                continue;

            GLuint baseidx = _rendererInfoArray[surf[i].rendererInfo].vertexIndex;
            for (int j = 0; j < surf[i].numedges - 2; ++j)
            {
                indexBuffer.insert(indexBuffer.end(), 
                    {baseidx, baseidx+1, baseidx+2});
                baseidx += 3;
            }
        }

        // visit far
        walkBspTree(node->children[!side], indexBuffer);
    }
}
