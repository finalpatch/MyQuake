#include "gllevel.h"

#include <algorithm>

extern "C"
{
#include "quakedef.h"
}

const static GLuint kLightmapAtlasSize = 1024;
const static GLuint kLightmapAtlasPadding = 1;

extern uint32_t vid_current_palette[256];

LevelRenderer::LevelRenderer() :
    _lightStyles(MAX_LIGHTSTYLES),
    _lightmapBuilder(std::make_unique<TextureAtlasBuilder<Texture::RGBA>>(kLightmapAtlasSize, kLightmapAtlasPadding))
{
}

void LevelRenderer::loadBrushModel(const model_s* brushModel)
{
    if (brushModel == cl.worldmodel)
    {
        _diffusemaps.clear();
        _diffusemaps.emplace_back(1, 1);
        for (int i = 0; i < brushModel->numtextures; ++i)
            loadTexture(brushModel->textures[i]);
    }
    else
    {
        auto loadTextureList = [this](texture_s* tex) {
            auto total = tex->anim_total;
            if (total == 0)
                total = 1;
            for(int j = 0; j < total; ++j)
            {
                if (tex->rendererData == 0)
                    loadTexture(tex);
                tex = tex->anim_next;
            }
        };

        for (int i = 0; i < brushModel->nummodelsurfaces; ++i)
        {
            auto& surface = brushModel->surfaces[brushModel->firstmodelsurface + i];
            auto* base = surface.texinfo->texture;
            loadTextureList(base);
            auto* alt = base->alternate_anims;
            if (alt)
                loadTextureList(alt);
        }
    }

    for (int i = 0; i < brushModel->nummodelsurfaces; ++i)
    {
        auto& surface = brushModel->surfaces[brushModel->firstmodelsurface + i];

        // read lightmaps
        TextureTile lightmapTile;
        static_assert(MAXLIGHTMAPS == 4, "we use rgba texture for the 4 lightmaps");
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

            lightmapTile = _lightmapBuilder->addImage(lightmapImage);
        }

        // store vertex index for this polygon
        surface.rendererData = _vertexData.size();

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
                    // compute texture coordinate
                    auto s =  v[i][0] * surface.texinfo->vecs[0][0] +
                             -v[i][2] * surface.texinfo->vecs[0][1] +
                              v[i][1] * surface.texinfo->vecs[0][2] +
                              surface.texinfo->vecs[0][3];
                    auto t =  v[i][0] * surface.texinfo->vecs[1][0] +
                             -v[i][2] * surface.texinfo->vecs[1][1] +
                              v[i][1] * surface.texinfo->vecs[1][2] +
                              surface.texinfo->vecs[1][3];

                    VertexAttr vert{
                        {v[i][0], v[i][1], v[i][2]},
                        {n[0], n[1], n[2]},
                        { (s - surface.texturemins[0]) / 16 + 1, (t - surface.texturemins[1]) / 16 + 1 },
                        { s / surface.texinfo->texture->width, t / surface.texinfo->texture->height },
                        { surface.styles[0], surface.styles[1], surface.styles[2], surface.styles[3] }
                    };

                    if (!lightmapTile.empty())
                        lightmapTile.translateCoordinate(vert.lightuv[0], vert.lightuv[1]);

                    _vertexData.push_back(std::move(vert));
                }

                previousVertexIndex = currentVertexIndex;
            }
        }
    }
}

void LevelRenderer::build()
{
    _vertexBuf = std::make_unique<GLBuffer<VertexAttr>>(_vertexData);
    _idxBuf = std::make_unique<GLBuffer<GLuint>>(nullptr, _vertexData.size(), kGlBufferDynamic);

    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(kVertexInputVertex);
    _vao->format(kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputVertex, *_vertexBuf, 0);

    _vao->enableAttrib(kVertexInputNormal);
    _vao->format(kVertexInputNormal, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputNormal, *_vertexBuf,
        offsetof(VertexAttr, normal));

    _vao->enableAttrib(kVertexInputLightTexCoord);
    _vao->format(kVertexInputLightTexCoord, 2, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputLightTexCoord, *_vertexBuf,
        offsetof(VertexAttr, lightuv));

    _vao->enableAttrib(kVertexInputDiffuseTexCoord);
    _vao->format(kVertexInputDiffuseTexCoord, 2, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(kVertexInputDiffuseTexCoord, *_vertexBuf,
        offsetof(VertexAttr, diffuseuv));

    _vao->enableAttrib(kVertexInputLightStyle);
    _vao->format(kVertexInputLightStyle, 4, GL_UNSIGNED_BYTE, GL_TRUE);
    _vao->vertexBuffer(kVertexInputLightStyle, *_vertexBuf,
        offsetof(VertexAttr, styles));

    _vao->indexBuffer(*_idxBuf);

    _lightmap = _lightmapBuilder->buildTexture(GL_CLAMP_TO_EDGE, GL_NEAREST, GL_LINEAR);

    _vertexData.clear();
    _lightmapBuilder.reset();
}

void LevelRenderer::renderSubmodel(const entity_s* entity)
{
    const model_s* submodel = entity->model;
    const float* origin = entity->origin;
    const float* angles = entity->angles;

    for (int i = 0; i < submodel->nummodelsurfaces; ++i)
    {
        const auto& surf = submodel->surfaces[submodel->firstmodelsurface + i];

        auto texture = textureAnimation(surf.texinfo->texture, entity->frame);
        auto textureChainIndex = texture->rendererData;
        auto& textureChain = _diffusemaps[textureChainIndex];

        GLuint baseidx = surf.rendererData;
        for (int j = 0; j < surf.numedges - 2; ++j)
        {
            textureChain.vertexes.insert(
                textureChain.vertexes.end(),
                {baseidx, baseidx+1, baseidx+2});
            baseidx += 3;
        }
    }

    glm::vec3 pos = qvec2glm(origin);
    glm::mat4 model =
        glm::translate(glm::mat4(), pos)
        * glm::rotate(glm::mat4(), glm::radians(angles[1]), {0, 1, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[0]), {1, 0, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[2]), {0, 0, 1});

    renderTextureChains(model);
}

void LevelRenderer::renderWorld(const entity_s* entity)
{
    animateLight();
    _framecount++;
	auto viewleaf = Mod_PointInLeaf(r_origin, cl.worldmodel);
    markLeaves(viewleaf);

    walkBspTree(cl.worldmodel->nodes, entity);

    renderTextureChains({});
}

void LevelRenderer::renderTextureChains(const glm::mat4& modelTrans)
{
    glm::vec3 origin = qvec2glm(cl_visedicts[0]->origin);
    glm::vec3 eyePos = qvec2glm(r_origin);
    glm::vec3 eyeDirection = qvec2glm(vpn);
    glm::mat4 model = modelTrans;
    glm::mat4 view = glm::lookAt(eyePos, eyePos + eyeDirection, qvec2glm(vup));

    _vao->bind();

    // bind the light map, and draw all normal walls
    TextureBinding lightmapBinding(*_lightmap, kTextureUnitLight);

    DefaultRenderPass::getInstance().setup(vid.width, vid.height, model, view,
        _lightStyles.data(), {0, 0, 0, 0});

    for (auto& textureChain: _diffusemaps)
    {
        if (!textureChain.vertexes.empty())
        {
            // bind diffuse map
            TextureBinding diffusemapBinding(textureChain.texture, kTextureUnitDiffuse);
            _idxBuf->update(textureChain.vertexes);
            glDrawElements(GL_TRIANGLES, textureChain.vertexes.size(), GL_UNSIGNED_INT, nullptr);
        }
    }

    // draw turb textures
    DefaultRenderPass::getInstance().setup(vid.width, vid.height, model, view,
        _lightStyles.data(), {0.5, 0.5, 0.5, 0}, kFlagTurbulence);
    for (auto& textureChain: _diffusemaps)
    {
        if (!textureChain.turbVertexes.empty())
        {
            // bind diffuse map
            TextureBinding diffusemapBinding(textureChain.texture, kTextureUnitDiffuse);
            _idxBuf->update(textureChain.turbVertexes);
            glDrawElements(GL_TRIANGLES, textureChain.turbVertexes.size(), GL_UNSIGNED_INT, nullptr);
        }
    }   

    // all done, clear texture chains for the next batch
    for (auto& textureChain: _diffusemaps)
    {
        textureChain.vertexes.clear();
        textureChain.turbVertexes.clear();
    }
}

void LevelRenderer::animateLight()
{
    int i = (int)(cl.time*10);
	for (int j=0 ; j<MAX_LIGHTSTYLES ; j++)
	{
		if (!cl_lightstyle[j].length)
		{
			_lightStyles[j] = 1.0f; // full bright
			continue;
		}
		int k = i % cl_lightstyle[j].length;
		k = cl_lightstyle[j].map[k] - 'a';
		_lightStyles[j] = (float)k / ('z' - 'a') * 2.0f; // up to double bright
	}
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

void LevelRenderer::walkBspTree(mnode_s *node, const entity_s* entity)
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
        walkBspTree(node->children[side], entity);

        // emit marked polygons
        for (int i = 0; i < node->numsurfaces; ++i)
        {
            const auto& surf = cl.worldmodel->surfaces[node->firstsurface + i];
            if(surf.visframe != _visframecount)
                continue;

            auto texture = textureAnimation(surf.texinfo->texture, entity->frame);
            auto textureChainIndex = texture->rendererData;
            auto& textureChain = _diffusemaps[textureChainIndex];

            GLuint baseidx = surf.rendererData;

            std::vector<GLuint>* vertexes;
            if (surf.flags & SURF_DRAWTURB)
                vertexes = &textureChain.turbVertexes;
            else
                vertexes = &textureChain.vertexes;

            for (int j = 0; j < surf.numedges - 2; ++j)
            {
                vertexes->insert(vertexes->end(), {baseidx, baseidx+1, baseidx+2});
                baseidx += 3;
            }
        }

        // visit far
        walkBspTree(node->children[!side], entity);
    }
}

int LevelRenderer::recursiveLightPoint (mnode_t *node, const float* start, const float* end)
{
	int			r;
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	unsigned	scale;
	int			maps;

	if (node->contents < 0)
		return -1;		// didn't hit anything

// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;

	if ( (back < 0) == side)
		return recursiveLightPoint (node->children[side], start, end);

	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;

// go down front side
	r = recursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something

	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing

// check for impact on this node

	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;

		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		r = 0;
		if (lightmap)
		{

			lightmap += dt * ((surf->extents[0]>>4)+1) + ds;

			for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
					maps++)
			{
                scale = unsigned(_lightStyles[surf->styles[maps]] * 255);
				r += *lightmap * scale;
				lightmap += ((surf->extents[0]>>4)+1) *
						((surf->extents[1]>>4)+1);
			}

			r >>= 8;
		}

		return r;
	}

// go down back side
	return recursiveLightPoint (node->children[!side], mid, end);
}

float LevelRenderer::lightPoint (const float* p)
{
	vec3_t		end;
	int			r;

	if (!cl.worldmodel->lightdata)
		return 1.0f;

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	r = recursiveLightPoint (cl.worldmodel->nodes, p, end);

	if (r == -1)
		r = 0;

	if (r < r_refdef.ambientlight)
		r = r_refdef.ambientlight;

	return r / 255.0f;
}

void LevelRenderer::loadTexture(texture_s* texture)
{
    Con_Printf("loading texture: %s\n", texture->name);
    texture->rendererData = _diffusemaps.size();
    unsigned w = texture->width;
    unsigned h = texture->height;
    _diffusemaps.emplace_back(w, h);
    std::vector<uint32_t> rgbtex(w * h);
    _diffusemaps.back().texture.setMaxMipLevel(MIPLEVELS - 1);
    for (int mip = 0; mip < MIPLEVELS; ++mip)
    {
        const uint8_t* pixels = reinterpret_cast<const uint8_t*>(texture) + texture->offsets[mip];
        for(int i = 0; i < w * h; ++i)
            rgbtex[i] = vid_current_palette[pixels[i]];
        if (mip == 0)
            _diffusemaps.back().texture.update(0, 0, w, h, rgbtex.data(), mip);
        else
            _diffusemaps.back().texture.addMipmap(w, h, rgbtex.data(), mip);
        w >>= 1; h >>= 1;
    }
}

const texture_s* LevelRenderer::textureAnimation(const texture_s* base, int frame)
{
	int		reletive;
	int		count;

	if (frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (!base->anim_total)
		return base;

	reletive = (int)(cl.time*10) % base->anim_total;

	count = 0;
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}
