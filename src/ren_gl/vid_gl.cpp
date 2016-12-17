#include <cstdint>
#include <SDL2/SDL.h>

extern "C"
{
#include "quakedef.h"
}

#define	BASEWIDTH  1280
#define	BASEHEIGHT 720

viddef_t	vid;
static uint32_t vid_current_palette[256];
qboolean isDedicated = qfalse;

static SDL_Window* win = nullptr;
static SDL_GLContext ctx = nullptr;

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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); 

    win = SDL_CreateWindow("Quake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        BASEWIDTH, BASEHEIGHT, SDL_WINDOW_OPENGL);
    if (!win) exit(-1);

    ctx = SDL_GL_CreateContext(win);
    if (!ctx) exit(-1);

	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.buffer = vid.conbuffer = nullptr;
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;
}

void VID_Shutdown (void)
{
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
}

void VID_Update (vrect_t *rects)
{
    SDL_GL_SwapWindow(win);
}

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}