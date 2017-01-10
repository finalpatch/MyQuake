#include "gllevel.h"
#include "glrenderprg.h"

#include <algorithm>

extern "C"
{
#include "quakedef.h"
}

const static GLuint kLightmapAtlasSize = 1024;
const static GLuint kLightmapAtlasPadding = 1;

LevelRenderer::LevelRenderer() :
    _lightStyles(MAX_LIGHTSTYLES),
    _lightmapBuilder(std::make_unique<TextureAtlasBuilder<Texture::RGBA>>(kLightmapAtlasSize, kLightmapAtlasPadding))
{
}

LevelRenderer::Submodel LevelRenderer::loadBrushModel(const model_s* brushModel)
{
    Submodel submodel;
    submodel.first =  _vertexBuffer.size();

    for (int i = 0; i < brushModel->nummodelsurfaces; ++i)
    {
        auto& surface = brushModel->surfaces[brushModel->firstmodelsurface + i];

        surface.rendererInfo = _rendererInfoArray.size();
        _rendererInfoArray.emplace_back();
        RendererInfo& surfaceRendererInfo = _rendererInfoArray.back();

        // read lightmaps
        static_assert(MAXLIGHTMAPS == 4); // we use rgba texture for the 4 lightmaps
        const uint8_t* lightmapData = surface.samples;
        if (lightmapData)
        {
            GLuint lightmapW = (surface.extents[0] >> 4) + 1;
            GLuint lightmapH = (surface.extents[1] >> 4) + 1;
            GLuint lightmapSize = lightmapW * lightmapH;

            Image<uint32_t> lightmapImage(lightmapW, lightmapH, nullptr);
            uint32_t* combinedLightmapData = lightmapImage.row(0);

            for (int lightmapIdx = 0; lightmapIdx < MAXLIGHTMAPS && surface.styles[lightmapIdx] != 255; ++lightmapIdx)
            {
                for (int i = 0; i < lightmapSize; ++i)
                    combinedLightmapData[i] |= lightmapData[i] << (24 - lightmapIdx * 8);
                lightmapData += lightmapSize;
            }

            surfaceRendererInfo.lightmap = _lightmapBuilder->addImage(lightmapImage);
        }

        // store vertex index
        surfaceRendererInfo.vertexIndex = _vertexBuffer.size();

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
                glm::vec3 v[3] = {
                    qvec2glm(brushModel->vertexes[firstVertexIndex].position),
                    qvec2glm(brushModel->vertexes[previousVertexIndex].position),
                    qvec2glm(brushModel->vertexes[currentVertexIndex].position)
                };
                glm::vec3 n = glm::normalize(qvec2glm(surface.plane->normal));
                if (surface.flags & SURF_PLANEBACK)
                    n *= -1;

                for (int i = 0; i < 3; ++i) 
                {
                    // vertex
                    _vertexBuffer.emplace_back(GLvec3{v[i][0], v[i][1], v[i][2]});
                    // surface normal
                    _normalBuffer.emplace_back(GLvec3{n[0], n[1], n[2]});

                    // texture coordinate
                    auto s =  v[i][0] * surface.texinfo->vecs[0][0] +
                             -v[i][2] * surface.texinfo->vecs[0][1] +
                              v[i][1] * surface.texinfo->vecs[0][2] +
                              surface.texinfo->vecs[0][3] -
                              surface.texturemins[0];
                    auto t =  v[i][0] * surface.texinfo->vecs[1][0] +
                             -v[i][2] * surface.texinfo->vecs[1][1] +
                              v[i][1] * surface.texinfo->vecs[1][2] +
                              surface.texinfo->vecs[1][3] -
                              surface.texturemins[1];
                    
                    // convert to lightmap coordinate
                    // lightmap is 1/16 res
                    // don't know why it needs the +1
                    // but otherwise the positions doesn't look right'
                    s = s / 16 + 1;
                    t = t / 16 + 1;

                    if (!surfaceRendererInfo.lightmap.empty())
                    {
                        surfaceRendererInfo.lightmap.translateCoordinate(s, t);
                    }
                    _texCoordBuffer.emplace_back(GLvec2{s, t});
                    _styleBuffer.emplace_back(std::array<GLubyte, 4>{
                        surface.styles[0], surface.styles[1],
                        surface.styles[2], surface.styles[3]});
                }

                previousVertexIndex = currentVertexIndex;
            }
        }
    }

    submodel.count = _vertexBuffer.size() - submodel.first;
    return submodel;
}

void LevelRenderer::build()
{
    _vtxBuf = std::make_unique<GLBuffer<GLvec3>>(_vertexBuffer.data(), _vertexBuffer.size());
    _nrmBuf = std::make_unique<GLBuffer<GLvec3>>(_normalBuffer.data(), _normalBuffer.size());
    _uvBuf  = std::make_unique<GLBuffer<GLvec2>>(_texCoordBuffer.data(), _texCoordBuffer.size());
    _styBuf = std::make_unique<GLBuffer<std::array<GLubyte, 4>>>(_styleBuffer.data(), _styleBuffer.size());
    _idxBuf = std::make_unique<GLBuffer<GLuint>>(nullptr, _vertexBuffer.size(), kGlBufferDynamic);

    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(kVertexInputVertex);
    _vao->format(kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputVertex, *_vtxBuf, sizeof(GLvec3));

    _vao->enableAttrib(kVertexInputNormal);
    _vao->format(kVertexInputNormal, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputNormal, *_nrmBuf, sizeof(GLvec3));

    _vao->enableAttrib(kVertexInputTexCoord);
    _vao->format(kVertexInputTexCoord, 2, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputTexCoord, *_uvBuf, sizeof(GLvec2));

    _vao->enableAttrib(kVertexInputStyle);
    _vao->format(kVertexInputStyle, 4, GL_UNSIGNED_BYTE, GL_TRUE);
    _vao->vertexBuffer(kVertexInputStyle, *_styBuf, sizeof(GLubyte)*4);

    _vao->indexBuffer(*_idxBuf);

    _lightmap = _lightmapBuilder->build();

    // release temp data
    std::vector<GLvec3> temp1, temp2;
    _vertexBuffer.swap(temp1);
    _normalBuffer.swap(temp2);
    std::vector<GLvec2> temp3;
    _texCoordBuffer.swap(temp3);
    _lightmapBuilder.reset();
}

void LevelRenderer::renderSubmodel(const Submodel& submodel, const float* origin, const float* angles)
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

    TextureBinding lightmapBinding(*_lightmap, GL_TEXTURE0);

    QuakeRenderProgram::getInstance().setup(vid.width, vid.height, model, view,
        _lightStyles.data(), {0, 0, 0, 0});
    _vao->bind();
    glDrawArrays(GL_TRIANGLES, submodel.first, submodel.count);
}

void LevelRenderer::renderWorld()
{
    animateLight();
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

    // bind the light map
    TextureBinding lightmapBinding(*_lightmap, GL_TEXTURE0);

    QuakeRenderProgram::getInstance().setup(vid.width, vid.height, model, view,
        _lightStyles.data(), {0, 0, 0, 0});
    _vao->bind();
    glDrawElements(GL_TRIANGLES, indexBuffer.size(), GL_UNSIGNED_INT, nullptr);
}

void LevelRenderer::animateLight()
{
    int i = (int)(cl.time*10);
	for (int j=0 ; j<MAX_LIGHTSTYLES ; j++)
	{
		if (!cl_lightstyle[j].length)
		{
			_lightStyles[j] = 1.0f;
			continue;
		}
		int k = i % cl_lightstyle[j].length;
		k = cl_lightstyle[j].map[k] - 'a';
		_lightStyles[j] = (float)k / ('z' - 'a');
        Con_Printf("%d: %.4f ", j,  _lightStyles[j]);
	}
    Con_Printf("\n");
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
