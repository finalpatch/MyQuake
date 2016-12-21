#include <cstdint>
#include <vector>
#include <SDL2/SDL.h>

extern "C"
{
#include "quakedef.h"
#include "d_local.h"
}

viddef_t	vid;				// global video state

std::vector<byte> renderBuffer;
std::vector<byte> warpBuffer;
std::vector<short> zBuffer;

byte* warpbuffer;

byte  surfcache[8*1024*1024];

static uint32_t vid_current_palette[256];

static SDL_Window*   win = NULL;
static SDL_Renderer* ren = NULL;
static SDL_Texture*  tex = NULL;

static setupSurfaces(int w, int h)
{
    renderBuffer.resize(w * h);
    warpBuffer.resize(w * h);
    zBuffer.resize(w * h);

	vid.maxwarpwidth = vid.width = vid.conwidth = w;
	vid.maxwarpheight = vid.height = vid.conheight = h;
    vid.buffer = vid.conbuffer = renderBuffer.data();
	vid.rowbytes = vid.conrowbytes = w;

    r_warpbuffer = warpBuffer.data();
    d_pzbuffer = zBuffer.data();

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) exit(-1);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!tex) exit(-1);
}

void VID_SetPalette (unsigned char *palette)
{
    for(int i = 0; i < 256; ++i)
    {
        vid_current_palette[i] =  
            uint32_t(palette[2]) | (uint32_t(palette[1]) << 8) |
            (uint32_t(palette[0]) << 16) | 0xff000000;
        palette += 3;
    }
}

void VID_ShiftPalette (unsigned char *palette)
{
    VID_SetPalette (palette);
}

void VID_Init (unsigned char *palette)
{
    int initialWidth = 1920;
    int initialHeight = 1080;

    win = SDL_CreateWindow("Quake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        initialWidth, initialHeight, SDL_WINDOW_RESIZABLE);
    if (!win) exit(-1);

    SDL_SetWindowMaximumSize(win, 1920, 1080);

    setupSurfaces(initialWidth, initialHeight);

	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;

	D_InitCaches (surfcache, sizeof(surfcache));
}

void VID_Shutdown (void)
{
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
}

void VID_Resize(int w, int h)
{
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);

    setupSurfaces(w, h);
}

void VID_Update (vrect_t *rects)
{
    byte* psrc = renderBuffer.data();
    uint32_t* pdst;
    int pitch;
    if (0 == SDL_LockTexture(tex, nullptr, (void**)&pdst, &pitch))
    {
        pitch /= sizeof(uint32_t);
        for(int row = 0; row < vid.height; ++row)
        {
            for(int i = 0; i < vid.width; ++i)
                pdst[i] = vid_current_palette[*psrc++];
            pdst += pitch;
        }
        SDL_UnlockTexture(tex);
    }
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}

qboolean isDedicated = qfalse;
