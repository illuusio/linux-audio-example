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
 * Pulseaudio development file (headers and libraries) http://www.freedesktop.org/wiki/Software/PulseAudio/ at least version 3.0
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with
 * gcc -g $(pkg-config --cflags --libs libpulse libpulse-simple) -lm -lsndfile libsndfile_pulse_blockrec.c -std=c99 -Wall -o libsndfile_pulse_blockrec
 *
 * Run with ./libsndfile_pulse_blockrec some.[wav/.flac/.aiff] (Warning! Will overwrite without warning!)
 */

#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <sndfile.h>

SNDFILE *m_SOutfile;
SF_INFO m_SSfinfo ;

int m_iLoop = 0;

#define PLAY_FRAMES_PER_BUFFER 44100

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
    m_iLoop = 1;
}

int main(int argc, char *argv[]) {
    long m_lReadcount = 0;
    pa_simple *l_SSimple = NULL;
    long l_lSizeonesec = (PLAY_FRAMES_PER_BUFFER * 2) * sizeof(float);

    /* Alloc size for one block */
    float *l_fSampleBlock = (float *)malloc(l_lSizeonesec);
    int l_iError = 0;
    struct sigaction l_SSa;

    static const pa_sample_spec l_SSs = {
         .format = PA_SAMPLE_FLOAT32,
         .rate = 44100,
         .channels = 2
    };

    printf("Record to file: '%s'\n", argv[1]);

    /*
      We use two channels
      Samplerate is 44100
      Wave 16 bit output format
    */
    m_SSfinfo.channels = 2;
    m_SSfinfo.samplerate = 44100;
    m_SSfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    /* Open file. Because this is just a example we asume
      What you are doing and give file first argument */
    if (! (m_SOutfile = sf_open(argv[1], SFM_WRITE, &m_SSfinfo))) {
        printf ("Not able to open output file %s.\n", argv[1]) ;
        sf_perror (NULL) ;
        return  1;
    }

    l_SSa.sa_flags = SA_SIGINFO;
    sigemptyset(&l_SSa.sa_mask);
    l_SSa.sa_sigaction = handler;

    if (sigaction(SIGINT, &l_SSa, NULL) == -1) {
        printf("Can't set SIGINT handler!\n");
        sf_close(m_SOutfile);
        return -1;
    }

    if (sigaction(SIGHUP, &l_SSa, NULL) == -1) {
        printf("Can't set SIGHUP handler!\n");
        sf_close(m_SOutfile);
        return -1;
    }

    /* Create a new playback stream */
    if (!(l_SSimple = pa_simple_new(NULL, "Simple example Pulseaudio record application", PA_STREAM_RECORD, NULL, "Record", &l_SSs, NULL, NULL, &l_iError))) {
       fprintf(stderr, "main: pa_simple_new() failed: %s\n", pa_strerror(l_iError));
       goto exit;
    }

    fflush(stdout);

    while(1) {
        if (pa_simple_read(l_SSimple, l_fSampleBlock, sizeof(l_fSampleBlock), &l_iError) < 0) {
            fprintf(stderr, "main: pa_simple_read() failed: %s\n", pa_strerror(l_iError));
            goto exit;
        }

        m_lReadcount = sf_write_float(m_SOutfile, l_fSampleBlock, l_lSizeonesec / 4);

        if(m_lReadcount <= 0) {
            printf("** Can't write to file!\n");
            goto exit;
        }

        if(m_iLoop) {
           break;
        }
    }

exit:
    sf_close(m_SOutfile);
    if (l_SSimple)
       pa_simple_free(l_SSimple);
    return 0;
}
