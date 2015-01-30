/*
 * Copyright (c) 2015 Tuukka Pasanen <tuukka.pasanen@ilmi.fi>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

 *
 * You need:
 * SDL1/2 (headers and libraries) http://www.libsdl.org/
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with libSDL1
 * gcc -g $(pkg-config --cflags --libs sdl) -lm -lsndfile libsndfile_sdl_play.c -std=c99 -Wall -o libsndfile_sdl_play
 *
 * Compile with libSDL2
 * gcc -g $(pkg-config --cflags --libs sdl2) -lm -lsndfile libsndfile_sdl_play.c -std=c99 -Wall -o libsndfile_sdl_play2

 * Run with ./libsndfile_sdl_play some.[wav/flac/aiff]
 */

#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <sndfile.h>
#include <signal.h>

SDL_AudioSpec m_SWantedSpec;
SDL_AudioSpec m_SSDLspec;
SNDFILE *m_SInfile = NULL;
SF_INFO m_SSinfo;
int l_iLoop = 0;
long m_iReadcount = 0;


/* Reques for writing length data */
static void sdlLibsndfileCb(void *userdata, Uint8 *stream, int len) {
    /* Just empty buffer for no reason.. fun yea */
    memset( stream, 0x00, len);

    /* Read with libsndfile */
#if SDL_MAJOR_VERSION == 2
    m_iReadcount = sf_read_float(m_SInfile, (float *)stream, len / 4);
#else
    m_iReadcount = sf_read_short(m_SInfile, (short int *)stream, len / 2);
#endif

    if( m_iReadcount <= 0 ) {
        l_iLoop = 1;
    }

}

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
    l_iLoop = 1;
}

int main(int argc, char *argv[]) {
    int retval = 0;
    struct sigaction l_SSa;
    SDL_Event       l_SEvent;

    /* Open file. Because this is just a example we asume
      What you are doing and give file first argument */
    if (! (m_SInfile = sf_open(argv[1], SFM_READ, &m_SSinfo))) {
        fprintf (stderr, "main: Not able to open input file %s.\n", argv[1]) ;
        sf_perror (NULL) ;
        return  1 ;
    }

    printf("Opened file: (%s)\n", argv[1]);

    l_SSa.sa_flags = SA_SIGINFO;
    sigemptyset(&l_SSa.sa_mask);
    l_SSa.sa_sigaction = handler;

    if (sigaction(SIGINT, &l_SSa, NULL) == -1) {
        fprintf(stderr, "main: Can't set SIGINT handler!\n");
        sf_close(m_SInfile);
        return -1;
    }

    if (sigaction(SIGHUP, &l_SSa, NULL) == -1) {
        fprintf(stderr, "main: Can't set SIGHUP handler!\n");
        sf_close(m_SInfile);
        goto exit;
    }

    /* SDL initialize */
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "main: Could not initialize SDL - %s\n", SDL_GetError());
        goto exit;
    }

    printf("We have samplerate: %5d and channels %2d\n", m_SSinfo.samplerate, m_SSinfo.channels);

    if( m_SSinfo.format & SF_FORMAT_PCM_S8 ) {
        printf("Subformat: 8-bit!\n");

    } else if( m_SSinfo.format & SF_FORMAT_PCM_16 ) {
        printf("Subformat: 16-bit!\n");

    } else if( m_SSinfo.format & SF_FORMAT_PCM_24 ) {
        printf("Subformat: 24-bit!\n");

    } else if( m_SSinfo.format & SF_FORMAT_PCM_32 ) {
        printf("Subformat: 32-bit!\n");

    } else if( m_SSinfo.format & SF_FORMAT_FLOAT ) {
        printf("Subformat: FLOAT!\n");

    } else if( m_SSinfo.format & SF_FORMAT_DOUBLE ) {
        printf("Subformat: DOUBLE!\n");
    }

    SDL_zero(m_SWantedSpec);
    SDL_zero(m_SSDLspec);

    m_SWantedSpec.freq = m_SSinfo.samplerate;
#if SDL_MAJOR_VERSION == 2
    m_SWantedSpec.format = AUDIO_F32LSB;
#else
    m_SWantedSpec.format = AUDIO_S16LSB;
#endif
    m_SWantedSpec.channels =  m_SSinfo.channels;
    m_SWantedSpec.silence = 0;
    m_SWantedSpec.samples = 1024;
    m_SWantedSpec.callback = sdlLibsndfileCb;
    m_SWantedSpec.userdata = m_SInfile;

    if(SDL_OpenAudio(&m_SWantedSpec, &m_SSDLspec) < 0) {
        fprintf(stderr, "main: SDL_OpenAudio: %s\n", SDL_GetError());
        goto exit;
    }

    SDL_PauseAudio(0);
    atexit(SDL_Quit);

    /* Loop until we exit */
    while(!l_iLoop) {
        SDL_PollEvent(&l_SEvent);
    }

exit:
    /* clean up and disconnect */
    printf("\nExit and clean\n");
    sf_close(m_SInfile);
    m_SInfile = NULL;

    return retval;
}


