/*
 * Most of the code is taken from these tutorials
 * http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncDeviceList/
 * http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncPlayback/
 * And added audio playing with libsndfile (FLAC, WAV, AIFF)
 *
 * You need:
 * Pulseaudio development file (headers and libraries) http://www.freedesktop.org/wiki/Software/PulseAudio/ at least version 3.0
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with libSDL1
 * gcc -g $(pkg-config --cflags --libs sdl) -lm -lsndfile libsndfile_sdl_read.c -std=c99 -Wall -o libsndfile_sdl_read
 *
 * Compile with libSDL2
 * gcc -g $(pkg-config --cflags --libs sdl2) -lm -lsndfile libsndfile_sdl_read.c -std=c99 -Wall -o libsndfile_sdl_read2

 * Run with ./libsndfile_sdl_read some.[wav/flac/aiff]
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

SDL_AudioSpec l_SWantedSpec;
SDL_AudioSpec l_SSDLspec;
SNDFILE *infile = NULL;
SF_INFO sfinfo;
int loop = 0;
long m_iReadcount = 0;


/* Reques for writing length data */
static void sdlLibsndfileCb(void *userdata, Uint8 *stream, int len) {
    /* Just empty buffer for no reason.. fun yea */
    memset( stream, 0x00, len);

    /* Read with libsndfile */
#if SDL_MAJOR_VERSION == 2
    m_iReadcount = sf_read_float(infile, (float *)stream, len / 4);
#else
    m_iReadcount = sf_read_short(infile, (short int *)stream, len / 2);
#endif

    if( m_iReadcount < 0 ) {
        loop = 1;
    }

}

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
    loop = 1;
}

int main(int argc, char *argv[]) {
    int retval = 0;
    struct sigaction sa;
    SDL_Event       event;

    /* Open file. Because this is just a example we asume
      What you are doing and give file first argument */
    if (! (infile = sf_open(argv[1], SFM_READ, &sfinfo))) {
        printf ("Not able to open input file %s.\n", argv[1]) ;
        sf_perror (NULL) ;
        return  1 ;
    }

    printf("Opened file: (%s)\n", argv[1]);

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf("Can't set SIGINT handler!\n");
        sf_close(infile);
        return -1;
    }

    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        printf("Can't set SIGHUP handler!\n");
        sf_close(infile);
        goto exit;
    }

    /* SDL initialize */
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        goto exit;
    }

    printf("We have samplerate: %5d and channels %2d\n", sfinfo.samplerate, sfinfo.channels);

    if( sfinfo.format & SF_FORMAT_PCM_S8 ) {
        printf("Subformat: 8-bit!\n");

    } else if( sfinfo.format & SF_FORMAT_PCM_16 ) {
        printf("Subformat: 16-bit!\n");

    } else if( sfinfo.format & SF_FORMAT_PCM_24 ) {
        printf("Subformat: 24-bit!\n");

    } else if( sfinfo.format & SF_FORMAT_PCM_32 ) {
        printf("Subformat: 32-bit!\n");

    } else if( sfinfo.format & SF_FORMAT_FLOAT ) {
        printf("Subformat: FLOAT!\n");

    } else if( sfinfo.format & SF_FORMAT_DOUBLE ) {
        printf("Subformat: DOUBLE!\n");
    }

    l_SWantedSpec.freq = sfinfo.samplerate;
#if SDL_MAJOR_VERSION == 2
    l_SWantedSpec.format = AUDIO_F32LSB;
#else
    l_SWantedSpec.format = AUDIO_S16LSB;
#endif
    l_SWantedSpec.channels =  sfinfo.channels;
    l_SWantedSpec.silence = 0;
    l_SWantedSpec.samples = 1024;
    l_SWantedSpec.callback = sdlLibsndfileCb;
    l_SWantedSpec.userdata = infile;

    if(SDL_OpenAudio(&l_SWantedSpec, &l_SSDLspec) < 0) {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        goto exit;
    }

    SDL_PauseAudio(0);
    atexit(SDL_Quit);

    /* Loop until we exit */
    while(!loop) {
        SDL_PollEvent(&event);
    }

exit:
    /* clean up and disconnect */
    printf("\nExit and clean\n");
    sf_close(infile);
    infile = NULL;

    return retval;
}


