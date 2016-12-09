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
// in_null.c -- for systems without a mouse

#include "quakedef.h"
#include <SDL2/SDL.h>

void IN_Init (void)
{
	SDL_SetRelativeMouseMode(true);
}

void IN_Shutdown (void)
{
	SDL_SetRelativeMouseMode(false);
}

void IN_Commands (void)
{
}

void IN_Move (usercmd_t *cmd)
{
    int mouse_x, mouse_y;
    SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
	if (mouse_x == 0 && mouse_y == 0)
		return;

	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;
   
	cl.viewangles[YAW] -= m_yaw.value * mouse_x;
	cl.viewangles[PITCH] += m_pitch.value * mouse_y;
	
	if (cl.viewangles[PITCH] > 80)
		cl.viewangles[PITCH] = 80;
	if (cl.viewangles[PITCH] < -70)
		cl.viewangles[PITCH] = -70;
}
