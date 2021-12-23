/*
 * Copyright (c) 2021 Tuukka Pasanen <tuukka.pasanen@ilmi.fi>
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
 * And added audio recording with libsndfile (FLAC, WAV, AIFF)
 *
 * You need:
 * Portaudio development file (headers and libraries) http://www.portaudio.com
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with
 * gcc -g $(pkg-config --cflags --libs portaudio-2.0) -lm -lsndfile libsndfile_port_rec.c -ansi -Wall -o libsndfile_port_rec
 *
 * Run with ./libsndfile_port_write some.wav (Warning! Will overwrite without warning!)
 */

#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <portaudio.h>
#include <sndfile.h>
#include <signal.h>

SNDFILE *outfile;
SF_INFO sfinfo ;

// Read one sec
#define READ_FRAMES_PER_BUFFER 44100
#define READ_WANTED_HOSTAPI "PulseAudio"
#define READ_DEVICE_NUM 5

/* Reques for writing length data */
static int paLibsndfileCb(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {
    float *in = (float*)inputBuffer;
    long writecount = 0;

    printf("Get frames Per Buffer: %ld\n", framesPerBuffer);

    /* Read with libsndfile */
    writecount = sf_write_float(outfile, in, framesPerBuffer * 2);

    /* File end if we read -1 */
    if(writecount <= 0) {
        printf("Can't write to file!\n");
        return paComplete;
    }

    return paContinue;
}

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
}

int main(int argc, char *argv[]) {
    int i = 0;
    PaStreamParameters inputParameters;
    PaStream *stream;
    const PaHostApiInfo* hostApiInfo = NULL;
    const PaDeviceInfo* deviceInfo = NULL;
    PaHostApiIndex hostApiIndex = 0;
    PaHostApiIndex selectedHostApiIndex = 0;
    const PaHostApiInfo* selectedHostApiInfo = NULL;
    PaDeviceIndex deviceIndex = 0;
    unsigned int hostApiCount = 0;
    PaError retval = 0;
    struct sigaction sa;

    /*
      We use two channels
      Samplerate is 44100
      Wave 16 bit output format
    */
    sfinfo.channels = 2;
    sfinfo.samplerate = 44100;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    /* Open file. Because this is just a example we asume
      What you are doing and give file first argument */
    if (! (outfile = sf_open(argv[1], SFM_WRITE, &sfinfo))) {
        printf ("Not able to open output file %s.\n", "input.wav") ;
        sf_perror (NULL) ;
        return  1 ;
    }

    printf("Opened file: (%s)\n", argv[1]);

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf("Can't set SIGINT handler!\n");
        sf_close(outfile);
        return -1;
    }

    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        printf("Can't set SIGHUP handler!\n");
        sf_close(outfile);
        return -1;
    }

    retval = Pa_Initialize();

    if(retval != paNoError) {
        fprintf(stderr, "Can't initialize Portaudio: %d\n", retval);
        goto exit;
    }

    hostApiCount = Pa_GetHostApiCount();

    printf("Portaudio version %d\n", Pa_GetVersion());
    printf("There is %d HostAPIs available\n", hostApiCount);

    inputParameters.device = paNoDevice;
    selectedHostApiIndex = Pa_GetDefaultHostApi();
    selectedHostApiInfo =  Pa_GetHostApiInfo(hostApiIndex);

    /* Query HostAPIs */
    for(hostApiIndex = 0; hostApiIndex < hostApiCount; hostApiIndex ++) {
        hostApiInfo =  Pa_GetHostApiInfo(hostApiIndex);
        printf(" - %d: Name: %s TypeID: %d", hostApiIndex, hostApiInfo->name, hostApiInfo->type);
        if(!strcmp(READ_WANTED_HOSTAPI, hostApiInfo->name)) {
            printf(" (This is wanted HostAPI)");
            selectedHostApiIndex = hostApiIndex;
            selectedHostApiInfo = hostApiInfo;
        }
        putc('\n', stdout);
    }

    printf("\nDevice name: %s\n", selectedHostApiInfo->name);

    /* Query devices on that HostAPI and */
    for(i = 0; i < selectedHostApiInfo->deviceCount; i ++) {
        deviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(selectedHostApiIndex, i);
        if(deviceIndex == paInvalidDevice) {
            printf("Too many devices queried\n");
            break;
        }

        deviceInfo = Pa_GetDeviceInfo(deviceIndex);
        printf(" - %d Device Index: %d Name: %s", i, deviceIndex, deviceInfo->name);
        if(READ_DEVICE_NUM == i) {
            printf(" (This is wanted device)");
            inputParameters.device = deviceIndex;
        }
        printf("\n       Input: %d (Low: %lf, High: %lf)\n", deviceInfo->maxInputChannels, deviceInfo->defaultLowInputLatency, deviceInfo->defaultHighInputLatency);
        printf("       Output: %d (Low: %lf, High: %lf)\n\n", deviceInfo->maxOutputChannels, deviceInfo->defaultLowOutputLatency, deviceInfo->defaultHighOutputLatency);
    }

    if(inputParameters.device == paNoDevice) {
        inputParameters.device = Pa_GetDefaultInputDevice(); /* default output device */
    }

    if (inputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        goto exit;
    }

    inputParameters.channelCount = 2;       /* stereo output */
    inputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowOutputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    retval = Pa_OpenStream(
                 &stream,
                 &inputParameters,
                 NULL, /* no output */
                 44100,
                 READ_FRAMES_PER_BUFFER,
                 paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                 paLibsndfileCb,
                 outfile);

    if(retval != paNoError) {
        fprintf(stderr, "Error: Cant open stream.\n");
        goto exit;
    }

    retval = Pa_StartStream(stream);

    if(retval != paNoError) {
        const char *errorStr = Pa_GetErrorText(retval);
        printf("Can't open device (%s)\n", errorStr);
        goto exit;
    }

    printf("Record 20 seconds.\n");
    Pa_Sleep(20 * 1000);

    retval = Pa_StopStream(stream);

    if(retval != paNoError) {
        fprintf(stderr, "Error: Cant stop stream.\n");
        goto exit;
    }

    retval = Pa_CloseStream(stream);

    if(retval != paNoError) {
        fprintf(stderr, "Error: Cant close stream.\n");
        goto exit;
    }

    Pa_Terminate();

exit:
    /* clean up and disconnect */
    printf("\nExit and clean\n");
    sf_close(outfile);
    Pa_Terminate();

    return retval;
}
