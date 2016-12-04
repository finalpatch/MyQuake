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

#include "quakedef.h"
#include <SDL2/SDL_audio.h>

SDL_AudioSpec want, have;
SDL_AudioDeviceID dev = 0;

const static size_t AUDIO_BUFFER_SAMPLES = 512;
volatile dma_t sn;

void SDLCALL render_audio (void *userdata, Uint8 * stream, int len)
{
	if ( shm ) {
		shm->buffer = stream;
		shm->samplepos += len/(shm->samplebits/8)/2;
		S_PaintChannels (shm->samplepos);
	}
}

qboolean SNDDMA_Init(void)
{
    shm = &sn;
    shm->splitbuffer = 0;
    shm->samplebits = 16;
    shm->speed = 22050;
    shm->channels = 2;
    shm->samples = AUDIO_BUFFER_SAMPLES * shm->channels;
    shm->samplepos = 0;
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = 1;
    shm->buffer = NULL;

    memset(&want, 0, sizeof(want));
    want.freq = shm->speed;
    want.format = AUDIO_S16;
    want.channels = shm->channels;
    want.samples = AUDIO_BUFFER_SAMPLES;
    want.callback = render_audio;

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (dev == 0) {
        Con_Printf("Failed to open audio: %s", SDL_GetError());
        return 0;
    }
    SDL_PauseAudioDevice(dev, 0); /* unpause */
    Con_Printf("Sound initialized %d %d %d.\n", have.freq, have.format, have.channels);

	return 1;
}

void SNDDMA_Shutdown(void)
{
	if (dev !=  0)
	{
        SDL_PauseAudioDevice(dev, 1);
        SDL_CloseAudioDevice(dev);
        dev = 0;
	}
}

int SNDDMA_GetDMAPos(void)
{
    return shm->samplepos;
}

void SNDDMA_Submit(void)
{
}
