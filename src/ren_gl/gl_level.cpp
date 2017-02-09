#include "gl_level.h"
#include <algorithm>

extern "C"
{
#include "quakedef.h"
}

const static GLuint kLightmapAtlasSize = 1024;
const static GLuint kLightmapAtlasPadding = 1;

extern uint32_t vid_current_palette[256];
extern glm::mat4 r_projectionMatrix;
extern glm::mat4 r_viewMatrix;

LevelRenderer::LevelRenderer() :
    _lightStyles(MAX_LIGHTSTYLES),
    _lightmapBuilder(std::make_unique<TextureAtlasBuilder<Texture::RGBA>>(kLightmapAtlasSize, kLightmapAtlasPadding))
{
}

void LevelRenderer::loadBrushModel(const model_s* brushModel)
{
    if (brushModel == cl.worldmodel)
    {
        _diffuseTextureChains.clear();
        _turbulenceTextureChains.clear();
        _skyTextureChains.clear();
        _skyBackgroundTextures.clear();
        _diffuseTextureChains.emplace_back(1, 1);
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
    _vertexBuf = std::make_unique<GLBuffer<VertexAttr, GL_ARRAY_BUFFER>>(_vertexData);
    _idxBuf = std::make_unique<GLBuffer<GLuint, GL_ELEMENT_ARRAY_BUFFER>>(nullptr, _vertexData.size(), kGlBufferDynamic);

    _vao = std::make_unique<VertexArray>();

    _vao->enableAttrib(DefaultRenderPass::kVertexInputVertex);
    _vao->format(DefaultRenderPass::kVertexInputVertex, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputVertex, *_vertexBuf, 0);

    _vao->enableAttrib(DefaultRenderPass::kVertexInputNormal);
    _vao->format(DefaultRenderPass::kVertexInputNormal, 3, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputNormal, *_vertexBuf,
        offsetof(VertexAttr, normal));

    _vao->enableAttrib(DefaultRenderPass::kVertexInputLightTexCoord);
    _vao->format(DefaultRenderPass::kVertexInputLightTexCoord, 2, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputLightTexCoord, *_vertexBuf,
        offsetof(VertexAttr, lightuv));

    _vao->enableAttrib(DefaultRenderPass::kVertexInputDiffuseTexCoord);
    _vao->format(DefaultRenderPass::kVertexInputDiffuseTexCoord, 2, GL_FLOAT, GL_FALSE);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputDiffuseTexCoord, *_vertexBuf,
        offsetof(VertexAttr, diffuseuv));

    _vao->enableAttrib(DefaultRenderPass::kVertexInputLightStyle);
    _vao->format(DefaultRenderPass::kVertexInputLightStyle, 4, GL_UNSIGNED_BYTE, GL_TRUE);
    _vao->vertexBuffer(DefaultRenderPass::kVertexInputLightStyle, *_vertexBuf,
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

    glm::vec3 pos = qvec2glm(origin);
    glm::mat4 model =
        glm::translate(glm::mat4(), pos)
        * glm::rotate(glm::mat4(), glm::radians(angles[1]), {0, 1, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[0]), {1, 0, 0})
        * glm::rotate(glm::mat4(), glm::radians(angles[2]), {0, 0, 1});

    // clip agains view frustum
    auto mvp = r_projectionMatrix * r_viewMatrix * model;
    auto planes = extractViewPlanes(mvp);
    auto box = qbox2glm(entity->model->mins, entity->model->maxs);
    if (!intersectFrustumAABB(planes, box))
        return;

    for (int i = 0; i < submodel->nummodelsurfaces; ++i)
    {
        const auto& surf = submodel->surfaces[submodel->firstmodelsurface + i];
        emitSurface(surf, entity->frame);
    }
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

void LevelRenderer::emitSurface(const msurface_s& surf, int frame)
{
    GLuint baseidx = surf.rendererData;
    auto texture = textureAnimation(surf.texinfo->texture, frame);
    auto textureChainIndex = texture->rendererData;

    std::vector<GLuint>* vertexes;
    if (surf.flags & SURF_DRAWTURB)
    {
        auto& textureChain = _turbulenceTextureChains[textureChainIndex];
        vertexes = &textureChain.vertexes;
    }
    else if (surf.flags & SURF_DRAWSKY)
    {
        auto& textureChain = _skyTextureChains[textureChainIndex];
        vertexes = &textureChain.vertexes;
    }
    else
    {
        auto& textureChain = _diffuseTextureChains[textureChainIndex];
        vertexes = &textureChain.vertexes;
    }

    for (int j = 0; j < surf.numedges - 2; ++j)
    {
        vertexes->insert(vertexes->end(), {baseidx, baseidx+1, baseidx+2});
        baseidx += 3;
    }
}

void LevelRenderer::renderTextureChains(const glm::mat4& modelMatrix)
{
    _vao->bind();

    DefaultRenderPass::getInstance().use();
    {
        // bind the light map, and draw all normal walls
        DefaultRenderPass::getInstance().setup(r_projectionMatrix, modelMatrix, r_viewMatrix,
            _lightStyles.data(), {0, 0, 0, 0});
        TextureBinding lightmapBinding(*_lightmap, DefaultRenderPass::kTextureUnitLight);
        for (auto& textureChain: _diffuseTextureChains)
        {
            if (!textureChain.vertexes.empty())
            {
                // bind diffuse map
                TextureBinding diffusemapBinding(textureChain.texture, DefaultRenderPass::kTextureUnitDiffuse);
                _idxBuf->update(textureChain.vertexes);
                glDrawElements(GL_TRIANGLES, textureChain.vertexes.size(), GL_UNSIGNED_INT, nullptr);
            }
        }
    }

    // draw turb textures
    DefaultRenderPass::getInstance().setup(r_projectionMatrix, modelMatrix, r_viewMatrix,
        _lightStyles.data(), {1.0, 1.0, 1.0, 0}, DefaultRenderPass::kFlagTurbulence);
    for (auto& textureChain: _turbulenceTextureChains)
    {
        if (!textureChain.vertexes.empty())
        {
            // bind diffuse map
            TextureBinding diffusemapBinding(textureChain.texture, DefaultRenderPass::kTextureUnitDiffuse);
            _idxBuf->update(textureChain.vertexes);
            glDrawElements(GL_TRIANGLES, textureChain.vertexes.size(), GL_UNSIGNED_INT, nullptr);
        }
    }        

    // draw sky textures
    glm::vec3 eyePos = qvec2glm(r_origin);
    SkyRenderPass::getInstance().use();
    SkyRenderPass::getInstance().setup(r_projectionMatrix, r_viewMatrix, glm::vec4(eyePos, 1.0));
    for (unsigned i = 0; i < _skyTextureChains.size(); ++i)
    {
        auto& textureChain = _skyTextureChains[i];
        if (!textureChain.vertexes.empty())
        {
            // bind diffuse map
            TextureBinding skyBinding0(textureChain.texture, SkyRenderPass::kTextureUnitSky0);
            TextureBinding skyBinding1(_skyBackgroundTextures[i], SkyRenderPass::kTextureUnitSky1);
            _idxBuf->update(textureChain.vertexes);
            glDrawElements(GL_TRIANGLES, textureChain.vertexes.size(), GL_UNSIGNED_INT, nullptr);
        }
    }

    // all done, clear texture chains for the next batch
    for (auto& textureChain: _diffuseTextureChains)
        textureChain.vertexes.clear();
    for (auto& textureChain: _turbulenceTextureChains)
        textureChain.vertexes.clear();
    for (auto& textureChain: _skyTextureChains)
        textureChain.vertexes.clear();
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
		_lightStyles[j] = (float)k / ('m' - 'a'); // up to double bright
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

    // clip agains view frustum
    auto mvp = r_projectionMatrix * r_viewMatrix;
    auto planes = extractViewPlanes(mvp);
    auto box = qbox2glm(node->minmaxs, node->minmaxs+3);
    if (!intersectFrustumAABB(planes, box))
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

            emitSurface(surf, entity->frame);
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
    if (!texture)
        return;

    Sys_Printf("loading texture: %s\n", texture->name);

    std::vector<TextureChain>* textureChains;
    if (texture->name[0] == '*')
        textureChains = &_turbulenceTextureChains;
    else if (texture->name[0] == 's' && texture->name[1] == 'k' && texture->name[2] == 'y')
        return loadSkyTexture(texture);
    else 
        textureChains = &_diffuseTextureChains;

    texture->rendererData = textureChains->size();
    unsigned w = texture->width;
    unsigned h = texture->height;
    textureChains->emplace_back(w, h);
    std::vector<uint32_t> rgbtex(w * h);
    textureChains->back().texture.setMaxMipLevel(MIPLEVELS - 1);
    for (int mip = 0; mip < MIPLEVELS; ++mip)
    {
        const uint8_t* pixels = reinterpret_cast<const uint8_t*>(texture) + texture->offsets[mip];
        for(int i = 0; i < w * h; ++i)
            rgbtex[i] = vid_current_palette[pixels[i]];
        if (mip == 0)
            textureChains->back().texture.update(0, 0, w, h, rgbtex.data(), mip);
        else
            textureChains->back().texture.addMipmap(w, h, rgbtex.data(), mip);
        w >>= 1; h >>= 1;
    }
}

void LevelRenderer::loadSkyTexture(texture_s* texture)
{
    texture->rendererData = _skyTextureChains.size();
    unsigned w = texture->width / 2;
    unsigned h = texture->height;
    _skyTextureChains.emplace_back(w, h);
    _skyTextureChains.back().texture.setMaxMipLevel(MIPLEVELS - 1);
    std::vector<uint32_t> rgbtex(w * h);
    _skyBackgroundTextures.emplace_back(GL_TEXTURE_2D, w, h, Texture::RGBA,
        GL_REPEAT, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST);
    _skyBackgroundTextures.back().setMaxMipLevel(MIPLEVELS - 1);
    for (int mip = 0; mip < MIPLEVELS; ++mip)
    {
        const uint8_t* pixels = reinterpret_cast<const uint8_t*>(texture) + texture->offsets[mip];
        // foreground
        for (unsigned row = 0; row < h; ++row)
        {
            for (unsigned col = 0; col < w; ++col)
            {
                auto clridx = pixels[row * w * 2 + col];
                rgbtex[row * w + col] = clridx == 0 ? 0 : vid_current_palette[pixels[row * w * 2 + col]];
            }
        }
        if (mip == 0)
            _skyTextureChains.back().texture.update(0, 0, w, h, rgbtex.data(), mip);
        else
            _skyTextureChains.back().texture.addMipmap(w, h, rgbtex.data(), mip);
        // background
        for (unsigned row = 0; row < h; ++row)
        {
            for (unsigned col = 0; col < w; ++col)
                rgbtex[row * w + col] = vid_current_palette[pixels[row * w * 2 + w + col]];
        }
        if (mip == 0)
            _skyBackgroundTextures.back().update(0, 0, w, h, rgbtex.data(), mip);
        else
            _skyBackgroundTextures.back().addMipmap(w, h, rgbtex.data(), mip);
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
