/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

extern "C"
{
#include "quakedef.h"
#include "errno.h"
#include <sys/time.h>
}

#include <SDL2/SDL.h>

#include <chrono>

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int findhandle (void)
{
	int             i;

	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int             pos;
	int             end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;

	return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime (char *path)
{
	FILE    *f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}

void Sys_mkdir (char *path)
{
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


void Sys_Error (const char *error, ...)
{
	va_list         argptr;

	printf ("Sys_Error: ");
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
	va_list         argptr;

	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
}

void Sys_Quit (void)
{
	exit (0);
}

double Sys_FloatTime (void)
{
	static auto timebase = std::chrono::steady_clock::now();
	std::chrono::duration<double> seconds = std::chrono::steady_clock::now() - timebase;
	return seconds.count();
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

static int MapKey( unsigned int sdlkey )
{
	switch(sdlkey)
	{
	default: return 0;
	case SDLK_RETURN:             return K_ENTER;
	case SDLK_ESCAPE:             return K_ESCAPE;
	case SDLK_BACKSPACE:          return K_BACKSPACE;
	case SDLK_TAB:                return K_TAB;
	case SDLK_SPACE:              return K_SPACE;
	case SDLK_EXCLAIM:            return '!';
	case SDLK_QUOTEDBL:           return '"';
	case SDLK_HASH:               return '#';
	case SDLK_PERCENT:            return '%';
	case SDLK_DOLLAR:             return '$';
	case SDLK_AMPERSAND:          return '&';
	case SDLK_QUOTE:              return '\'';
	case SDLK_LEFTPAREN:          return '(';
	case SDLK_RIGHTPAREN:         return ')';
	case SDLK_ASTERISK:           return '*';
	case SDLK_PLUS:               return '+';
	case SDLK_COMMA:              return ',';
	case SDLK_MINUS:              return '-';
	case SDLK_PERIOD:             return '.';
	case SDLK_SLASH:              return '/';
	case SDLK_0:                  return '0';
	case SDLK_1:                  return '1';
	case SDLK_2:                  return '2';
	case SDLK_3:                  return '3';
	case SDLK_4:                  return '4';
	case SDLK_5:                  return '5';
	case SDLK_6:                  return '6';
	case SDLK_7:                  return '7';
	case SDLK_8:                  return '8';
	case SDLK_9:                  return '9';
	case SDLK_COLON:              return ':';
	case SDLK_SEMICOLON:          return ';';
	case SDLK_LESS:               return '<';
	case SDLK_EQUALS:             return '=';
	case SDLK_GREATER:            return '>';
	case SDLK_QUESTION:           return '?';
	case SDLK_AT:                 return '@';
	case SDLK_LEFTBRACKET:        return '[';
	case SDLK_BACKSLASH:          return '\\';
	case SDLK_RIGHTBRACKET:       return ']';
	case SDLK_CARET:              return '^';
	case SDLK_UNDERSCORE:         return '_';
	case SDLK_BACKQUOTE:          return '`';
	case SDLK_a:                  return 'a';
	case SDLK_b:                  return 'b';
	case SDLK_c:                  return 'c';
	case SDLK_d:                  return 'd';
	case SDLK_e:                  return 'e';
	case SDLK_f:                  return 'f';
	case SDLK_g:                  return 'g';
	case SDLK_h:                  return 'h';
	case SDLK_i:                  return 'i';
	case SDLK_j:                  return 'j';
	case SDLK_k:                  return 'k';
	case SDLK_l:                  return 'l';
	case SDLK_m:                  return 'm';
	case SDLK_n:                  return 'n';
	case SDLK_o:                  return 'o';
	case SDLK_p:                  return 'p';
	case SDLK_q:                  return 'q';
	case SDLK_r:                  return 'r';
	case SDLK_s:                  return 's';
	case SDLK_t:                  return 't';
	case SDLK_u:                  return 'u';
	case SDLK_v:                  return 'v';
	case SDLK_w:                  return 'w';
	case SDLK_x:                  return 'x';
	case SDLK_y:                  return 'y';
	case SDLK_z:                  return 'z';
	case SDLK_F1:                 return K_F1;
	case SDLK_F2:                 return K_F2;
	case SDLK_F3:                 return K_F3;
	case SDLK_F4:                 return K_F4;
	case SDLK_F5:                 return K_F5;
	case SDLK_F6:                 return K_F6;
	case SDLK_F7:                 return K_F7;
	case SDLK_F8:                 return K_F8;
	case SDLK_F9:                 return K_F9;
	case SDLK_F10:                return K_F10;
	case SDLK_F11:                return K_F11;
	case SDLK_F12:                return K_F12;
	case SDLK_PAUSE:              return K_PAUSE;
	case SDLK_INSERT:             return K_INS;
	case SDLK_HOME:               return K_HOME;
	case SDLK_PAGEUP:             return K_PGUP;
	case SDLK_END:                return K_END;
	case SDLK_PAGEDOWN:           return K_PGDN;
	case SDLK_RIGHT:              return K_RIGHTARROW;
	case SDLK_LEFT:               return K_LEFTARROW;
	case SDLK_DOWN:               return K_DOWNARROW;
	case SDLK_UP:                 return K_UPARROW;
	case SDLK_LCTRL:              return K_CTRL;
	case SDLK_LSHIFT:             return K_SHIFT;
	case SDLK_LALT:               return K_ALT;
	case SDLK_RCTRL:              return K_CTRL;
	case SDLK_RSHIFT:             return K_SHIFT;
	case SDLK_RALT:               return K_ALT;
	}
}

void Sys_SendKeyEvents (void)
{
	SDL_Event event;
	if (!SDL_PollEvent(&event))
		return;
	switch (event.type)
	{
	case SDL_KEYUP:
		Key_Event(MapKey(event.key.keysym.sym), false);
		break;
	case SDL_KEYDOWN:
		Key_Event(MapKey(event.key.keysym.sym), true);
		break;
	default:
		break;
	}
}

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

//=============================================================================

int main (int argc, char **argv)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	atexit(SDL_Quit);

	static quakeparms_t    parms;
    double		time, oldtime, newtime;

	parms.memsize = 16*1024*1024;
	parms.membase = malloc (parms.memsize);
	parms.basedir = ".";

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	printf ("Host_Init\n");
	Host_Init (&parms);

    oldtime = Sys_FloatTime () - 0.1;
    while (1)
    {
		// find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;

        /* if (cls.state == ca_dedicated) */
        /* {   // play vcrfiles at max speed */
        /*     if (time < sys_ticrate.value && (vcrFile == -1 || recording) ) */
        /*     { */
		/* 		usleep(1); */
        /*         continue;       // not time to run a server only tic yet */
        /*     } */
        /*     time = sys_ticrate.value; */
        /* } */

        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        Host_Frame (time);

// graphic debugging aids
        /* if (sys_linerefresh.value) */
        /*     Sys_LineRefresh (); */
    }
	return 0;
}
