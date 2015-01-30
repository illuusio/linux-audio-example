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
 * Libao development file (headers and libraries) https://xiph.org/ao/doc/overview.html
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with
 * gcc -g $(pkg-config --cflags --libs ao) -lm -lsndfile libsndfile_libao_blockplay.c -std=c99 -Wall -o libsndfile_libao_blockplay
 *
 * Run with ./libsndfile_libao_blockplay some.[wav/.flac/.aiff] (Warning! Will overwrite without warning!)
 */

#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ao/ao.h>
#include <sndfile.h>
#include <signal.h>

SNDFILE *m_SInfile;
SF_INFO m_SSfinfo ;

int m_iLoop = 0;

#define PLAY_FRAMES_PER_BUFFER 44100

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
    m_iLoop = 1;
}

int main(int argc, char *argv[]) {
    long l_lReadcount = 0;
    int l_iDriverNum;
    ao_device *l_SAODev = NULL;
    ao_sample_format l_SAOFormat;
    long l_lSizeonesec = (PLAY_FRAMES_PER_BUFFER * 2) * sizeof(short);

    /* Alloc size for one block */
    short *l_iSampleBlock = (short *)malloc(l_lSizeonesec);
    struct sigaction l_SSa;

    printf("Playing file: '%s'\n", argv[1]);

    /* Open file. Because this is just a example we asume
      What you are doing and give file first argument */
    if (! (m_SInfile = sf_open(argv[1], SFM_READ, &m_SSfinfo))) {
        printf ("Not able to open input file %s.\n", argv[1]) ;
        sf_perror (NULL) ;
        return  1 ;
    }

    l_SSa.sa_flags = SA_SIGINFO;
    sigemptyset(&l_SSa.sa_mask);
    l_SSa.sa_sigaction = handler;

    if (sigaction(SIGINT, &l_SSa, NULL) == -1) {
        printf("Can't set SIGINT handler!\n");
        sf_close(m_SInfile);
        return -1;
    }

    if (sigaction(SIGHUP, &l_SSa, NULL) == -1) {
        printf("Can't set SIGHUP handler!\n");
        sf_close(m_SInfile);
        return -1;
    }

    ao_initialize();
    l_iDriverNum = ao_default_driver_id();

    /* set the output format and open the output device */
    l_SAOFormat.bits = 16;
    l_SAOFormat.rate = m_SSfinfo.samplerate;
    l_SAOFormat.channels = m_SSfinfo.channels;
    l_SAOFormat.byte_format = AO_FMT_NATIVE;
    l_SAOFormat.matrix = 0;

    l_SAODev = ao_open_live(l_iDriverNum, &l_SAOFormat, NULL);

    if(l_SAODev == NULL)
    {
      fprintf(stderr,"Can't open audio output %d (errno: %d)\n", l_iDriverNum, errno);
      goto exit;
    }

    fflush(stdout);

    while(1) {
        l_lReadcount = sf_read_short(m_SInfile, l_iSampleBlock, l_lSizeonesec / 4);

        if(l_lReadcount <= 0) {
            printf("** File has ended (readcount: %ld/%ld)!\n", l_lReadcount, (l_lSizeonesec / 4));
            break;
        }

        ao_play(l_SAODev, (char *)l_iSampleBlock, (l_lSizeonesec / 2));

        if(m_iLoop) {
           break;
        }
    }

exit:
    sf_close(m_SInfile);
    if(l_SAODev != NULL)
    {
      ao_close(l_SAODev);
    }
    ao_shutdown();
    return 0;
}
