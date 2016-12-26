#include "gllevel.h"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern "C"
{
#include "quakedef.h"
}

LevelRenderer::LevelRenderer(const model_s* levelModel)
{
}

void LevelRenderer::render()
{
	auto viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);
    _oldviewleaf = viewleaf;

    //R_RenderWorld();
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
