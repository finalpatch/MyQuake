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

#include <SDL2/SDL_mixer.h>
#include "quakedef.h"

static Mix_Music* tracks[12];

void CDAudio_Play(byte track, qboolean looping)
{
	if (track <= 11 && tracks[track])
	{
		Mix_PlayMusic(tracks[track], looping ? -1 : 1);
	}
}


void CDAudio_Stop(void)
{
	Mix_PauseMusic();
	Mix_RewindMusic();
}


void CDAudio_Pause(void)
{
	Mix_PauseMusic();
}


void CDAudio_Resume(void)
{
	Mix_ResumeMusic();
}


void CDAudio_Update(void)
{
	static float currentvolume = 0.0f;
	if (currentvolume != bgmvolume.value)
	{
		currentvolume = bgmvolume.value;
		Mix_VolumeMusic(MIX_MAX_VOLUME * bgmvolume.value);
	}
}


int CDAudio_Init(void)
{
	char filename[32];

	if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG)
		Con_Printf("init ogg failed\n");
	if (Mix_OpenAudio(44100, AUDIO_S16, 2, 4096) != 0)
		Con_Printf("init mixer failed\n");

	for (int i = 2; i < 12; ++i)
	{
		sprintf(filename, "id1/music/track%02d.ogg", i);
		tracks[i] = Mix_LoadMUS(filename);
		Con_Printf("loading track %s ... %s\n", filename, tracks[i] ? "done" : "failed");
	}

	Mix_VolumeMusic(MIX_MAX_VOLUME * bgmvolume.value);
	
	return 0;
}


void CDAudio_Shutdown(void)
{
	for (int i = 2; i < 12; ++i)
	{
		Mix_FreeMusic(tracks[i]);
		tracks[i] = NULL;
	}
	Mix_CloseAudio();
	Mix_Quit();
}