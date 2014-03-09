/*
 * Most of the code is taken from these tutorials
 * https://www.assembla.com/code/portaudio/subversion/nodes/1922/portaudio/trunk/examples/paex_sine.c
 * And added audio playing with libsndfile (FLAC, WAV, AIFF)
 *
 * You need:
 * Portaudio development file (headers and libraries) http://www.portaudio.com
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with
 * gcc -g $(pkg-config --cflags --libs portaudio-2.0) -lm -lsndfile libsndfile_port_write.c -ansi -Wall -o libsndfile_port_write
 *
 * Run with ./libsndfile_port_write some.[wav/.flac/.aiff]
 */

#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <portaudio.h>
#include <sndfile.h>
#include <signal.h>

SNDFILE *infile;
SF_INFO sfinfo ;

/* Reques for writing length data */
static int paLibsndfileCb( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData ) {
    float *out = (float*)outputBuffer;
    long readcount = 0;

    /* Just empty buffer for no reason.. fun yea */
    memset( out, 0x00, (framesPerBuffer * 2) * 4);

    /* Read with libsndfile */
    readcount = sf_read_float(infile, out, framesPerBuffer * 2);

    /* File end if we read -1 */
    if( readcount <= 0 ) {
        printf("File end!\n");
        return paComplete;
    }

    return paContinue;
}

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
}

int main(int argc, char *argv[]) {
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError retval = 0;
    struct sigaction sa;

    /* Open file. Because this is just a example we asume
      What you are doing and give file first argument */
    if (! (infile = sf_open(argv[1], SFM_READ, &sfinfo))) {
        printf ("Not able to open input file %s.\n", "input.wav") ;
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

    retval = Pa_Initialize();

    if( retval != paNoError ) {
        goto exit;
    }

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */

    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        goto exit;
    }

    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    retval = Pa_OpenStream(
                 &stream,
                 NULL, /* no input */
                 &outputParameters,
                 44100,
                 4096,
                 paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                 paLibsndfileCb,
                 infile );

    if( retval != paNoError ) {
        goto exit;
    }

    retval = Pa_StartStream( stream );

    if( retval != paNoError ) {
        goto exit;
    }

    printf("Play maximum 360 seconds.\n");
    Pa_Sleep( 1000 * 360 );

    retval = Pa_StopStream( stream );

    if( retval != paNoError ) {
        goto exit;
    }

    retval = Pa_CloseStream( stream );

    if( retval != paNoError ) {
        goto exit;
    }

    Pa_Terminate();

exit:
    /* clean up and disconnect */
    printf("\nExit and clean\n");
    sf_close(infile);
    Pa_Terminate();

    return retval;
}


