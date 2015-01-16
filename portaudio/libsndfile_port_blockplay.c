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
 * Most of the code is taken from these tutorials
 * https://www.assembla.com/code/portaudio/subversion/nodes/1922/portaudio/trunk/examples/paex_sine.c
 * And added audio playing with libsndfile (FLAC, WAV, AIFF)
 *
 * You need:
 * Portaudio development file (headers and libraries) http://www.portaudio.com
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with
 * gcc -g $(pkg-config --cflags --libs portaudio-2.0) -lm -lsndfile libsndfile_port_blockplay.c -ansi -Wall -o libsndfile_port_blockplay
 *
 * Run with ./libsndfile_port_read some.[wav/.flac/.aiff] (Warning! Will overwrite without warning!)
 */

#define _BSD_SOURCE
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <portaudio.h>
#include <sndfile.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

SNDFILE *infile;
SF_INFO sfinfo ;

#define PLAY_FRAMES_PER_BUFFER 44100

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
}

int main(int argc, char *argv[]) {
    long i = 0;
    long readcount = 0;
    PaStreamParameters outputParameters;
    PaStream *stream = NULL;
    long sizeonesec = (PLAY_FRAMES_PER_BUFFER * 2) * sizeof(float);

    /* Alloc size for one block */
    float *sampleBlock = (float *)malloc(sizeonesec);
    PaError retval = 0;
    struct sigaction sa;

    printf("Playing file: '%s'\n", argv[1]);

    /* Open file. Because this is just a example we asume
      What you are doing and give file first argument */
    if (! (infile = sf_open(argv[1], SFM_READ, &sfinfo))) {
        printf ("Not able to open input file %s.\n", argv[1]) ;
        sf_perror (NULL) ;
        return  1 ;
    }

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
        return -1;
    }


    /* -- initialize PortAudio -- */
    retval = Pa_Initialize();

    if(retval != paNoError) {
        goto exit;
    }

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */

    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        goto exit;
    }

    /* -- setup stream -- */
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    retval = Pa_OpenStream(
                 &stream,
                 NULL, /* no input */
                 &outputParameters,
                 44100,
                 PLAY_FRAMES_PER_BUFFER,
                 paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                 NULL,
                 infile);

    if(retval != paNoError) {
        goto exit;
    }

    /* -- start stream -- */
    retval = Pa_StartStream(stream);

    if(retval != paNoError) {
        goto exit;
    }

    printf("Wire on. Will run one minute.\n");
    fflush(stdout);

    /* -- Here's the loop where we pass data from input to output -- */
    for(i = 0; i < ((60 * sizeonesec) / (44100 * 2)); ++i) {
        readcount = sf_read_float(infile, sampleBlock, sizeonesec / 4);

        if(readcount <= 0) {
            printf("** File has ended!\n");
            goto exit;
        }

        /* retval = Pa_WriteStream(stream, (void *)sampleBlock, sizeonesec / 8); */
        retval = Pa_WriteStream(stream, (void *)sampleBlock, sizeonesec / 8);

        if(retval != paNoError) {
            printf("** Can't write file to output!\n");
            goto exit;
        }
    }

exit:
    sf_close(infile);
    retval = Pa_StopStream(stream);
    retval = Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;

}
