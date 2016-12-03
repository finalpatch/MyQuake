extern "C"
{
#include "quakedef.h"
#include "d_local.h"
}

#include <SDL2/SDL.h>

viddef_t	vid;				// global video state

#define	BASEWIDTH  1280
#define	BASEHEIGHT 720

byte  vid_buffer[BASEWIDTH*BASEHEIGHT];
short zbuffer[BASEWIDTH*BASEHEIGHT];
byte  surfcache[8*1024*1024];

unsigned short d_8to16table[256];
unsigned       d_8to24table[256];

byte vid_current_palette[256*3];
byte vid_rgb_buffer[BASEWIDTH*BASEHEIGHT*4];

SDL_Window*   win = NULL;
SDL_Renderer* ren = NULL;
SDL_Texture*  tex = NULL;

void VID_SetPalette (unsigned char *palette)
{
	if (palette != vid_current_palette)
		Q_memcpy(vid_current_palette, palette, sizeof(vid_current_palette));
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
    for(int i = 0; i < BASEWIDTH*BASEHEIGHT; ++i)
    {
        byte* rgb = vid_current_palette + vid_buffer[i] * 3;
        vid_rgb_buffer[i*4+0] = rgb[2];
        vid_rgb_buffer[i*4+1] = rgb[1];
        vid_rgb_buffer[i*4+2] = rgb[0];
        vid_rgb_buffer[i*4+3] = 255;
    }
    SDL_UpdateTexture(tex, nullptr, vid_rgb_buffer, BASEWIDTH*4);
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
