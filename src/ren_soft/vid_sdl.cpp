extern "C"
{
#include "quakedef.h"
#include "d_local.h"
}

#include <cstdint>

#include <SDL2/SDL.h>

viddef_t	vid;				// global video state

#define	BASEWIDTH  1280
#define	BASEHEIGHT 720

byte  vid_buffer[BASEWIDTH*BASEHEIGHT];
short zbuffer[BASEWIDTH*BASEHEIGHT];
byte  surfcache[8*1024*1024];

unsigned short d_8to16table[256];
unsigned       d_8to24table[256];

static uint32_t vid_current_palette[256];

static SDL_Window*   win = NULL;
static SDL_Renderer* ren = NULL;
static SDL_Texture*  tex = NULL;

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
    if (SDL_CreateWindowAndRenderer(BASEWIDTH, BASEHEIGHT, 0, &win, &ren) < 0)
        exit(-1);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, BASEWIDTH, BASEHEIGHT);
    if (!tex)
        exit(-1);

	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vid_buffer;
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;

	d_pzbuffer = zbuffer;
	D_InitCaches (surfcache, sizeof(surfcache));
}

void VID_Shutdown (void)
{
    if (tex)
        SDL_DestroyTexture(tex);
    if (ren)
        SDL_DestroyRenderer(ren);
    if (win)
        SDL_DestroyWindow(win);
}

void VID_Update (vrect_t *rects)
{
    byte* psrc = vid_buffer;
    uint32_t* pdst;
    int pitch;
    if (0 == SDL_LockTexture(tex, nullptr, (void**)&pdst, &pitch))
    {
        pitch /= sizeof(uint32_t);
        for(int row = 0; row < BASEHEIGHT; ++row)
        {
            for(int i = 0; i < BASEWIDTH; ++i)
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
