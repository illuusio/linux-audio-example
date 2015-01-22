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
 * Resources used as study for this example are
 * http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncDeviceList/
 * http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncPlayback/
 * And added audio playing with libsndfile (FLAC, WAV, AIFF)
 *
 * You need:
 * Pulseaudio development file (headers and libraries) http://www.freedesktop.org/wiki/Software/PulseAudio/ at least version 3.0
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with
 * gcc -g $(pkg-config --cflags --libs libpulse) -lm -lsndfile libsndfile_pulse_play.c -std=c99 -Wall -o libsndfile_pulse_play
 *
 * Run with ./libsndfile_pulse_play some.[wav/flac/aiff]
 */

#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <sndfile.h>

typedef struct pulseinfo {
  char name[512];
  uint32_t card;
  pa_sample_spec sample_spec;
  pa_channel_map channel_map;
} pulseinfo;
  
static int m_iLatency = 20000; /* start latency in micro seconds */
float m_fSampledata[88100 * sizeof(float) * 2];
static pa_buffer_attr m_SBufAttr;
static int m_iUnderflows = 0;
static pa_sample_spec m_SSs;
SNDFILE *m_SInfile = NULL;
SF_INFO m_SSfinfo;
int m_iLoop = 0;
pulseinfo m_SSinkList[1024];
pulseinfo m_SSourceList[1024];
int m_iSinkCount = -1;
int m_iSourceCount = -1;


/* When context change state this called */
void pa_state_cb(pa_context *c, void *userdata) {
    pa_context_state_t l_iState;
    int *l_iPaReady = userdata;
    l_iState = pa_context_get_state(c);

    switch  (l_iState) {
            /* These are just here for reference */
        case PA_CONTEXT_UNCONNECTED:
            printf("pa_state_cb: PA_CONTEXT_UNCONNECTED\n");
            break;

        case PA_CONTEXT_CONNECTING:
            printf("pa_state_cb: PA_CONTEXT_CONNECTING\n");
            break;

        case PA_CONTEXT_AUTHORIZING:
            printf("pa_state_cb: PA_CONTEXT_AUTHORIZING\n");
            break;

        case PA_CONTEXT_SETTING_NAME:
            printf("pa_state_cb: PA_CONTEXT_NAME\n");
            break;

        default:
            break;

        case PA_CONTEXT_FAILED:
            printf("pa_state_cb: PA_CONTEXT_FAILED\n");
            *l_iPaReady = 2;
            break;

        case PA_CONTEXT_TERMINATED:
            printf("pa_state_cb: PA_CONTEXT_TERMINATED\n");
            *l_iPaReady = 2;
            break;

        case PA_CONTEXT_READY:
            printf("pa_state_cb: PA_CONTEXT_READY\n");

            *l_iPaReady = 1;
            break;
    }
}

/* List sinks in our system (For playback) */
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata) {
    char l_strSampleSpec[1024];
    char l_strChannelMap[1024];
    char l_strFormatInfo[1024];
    int i = 0;

    if( eol ) {
        return;
    }

    m_iSinkCount ++;
    strncpy((char *)m_SSinkList[m_iSinkCount].name, l->name, strlen(l->name));
    m_SSinkList[m_iSinkCount].card = l->card;
    m_SSinkList[m_iSinkCount].channel_map = l->channel_map;
    m_SSinkList[m_iSinkCount].sample_spec = l->sample_spec;

    printf("SINK %d--------------------------------------------------------------------------\n", l->card);
    printf("\tSink Name: '%s'\n\tDriver name: '%s'\n\t", l->name, l->driver);
    printf("Description: '%s'\n\t",l->description);
    printf("Latency: '%ld' Configured Latency: '%ld'\n\t", l->latency, l->configured_latency);
    pa_sample_spec_snprint(l_strSampleSpec, 1024, &l->sample_spec);
    pa_channel_map_snprint(l_strChannelMap, 1024, &l->channel_map);
    printf("Sample spec: '%s'\n\t", l_strSampleSpec);
    printf("Channel map: '%s'\n\n\t", l_strChannelMap);

    if( l->active_port != NULL )
    {
       printf("Active Port Name: '%s'\n\tDescription '%s'\n\tPriority: '%d'\n\tAvailable:: '%d'\n", l->active_port->name, l->active_port->description, l->active_port->priority, l->active_port->available);
    }

    printf("\nPorts:\n");

    for(i=0; i < l->n_ports; i++) {
	printf("\t\t%d: Port Name: '%s'\n\t\tDescription '%s'\n\t\tPriority: '%d'\n\t\tAvailable:: '%d'\n", i, l->ports[i]->name, l->ports[i]->description, l->ports[i]->priority, l->ports[i]->available);
    }

    printf("\nSupported formats:\n");

    for(i=0; i < l->n_formats; i++) {
        pa_format_info_snprint(l_strFormatInfo, 1024, l->formats[i]);
	printf("\t\t%d: Supported format: '%s'\n", i, l_strFormatInfo);
    }
    printf("\n");

}

/* List sources in our system (For recording) */
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata) {
    char l_strSampleSpec[1024];
    char l_strChannelMap[1024];
    char l_strFormatInfo[1024];
    int i = 0;

    if( eol ) {
        return;
    }

    m_iSourceCount ++;
    strncpy((char *)m_SSourceList[m_iSourceCount].name, l->name, strlen(l->name));
    m_SSourceList[m_iSourceCount].card = l->card;
    m_SSourceList[m_iSourceCount].channel_map = l->channel_map;
    m_SSourceList[m_iSourceCount].sample_spec = l->sample_spec;

    printf("SOURCE %d--------------------------------------------------------------------------\n", l->card);
    printf("\tSource Name: '%s'\n\tDriver name: '%s'\n\t", l->name, l->driver);
    printf("Description: '%s'\n\t",l->description);
    printf("Latency: '%ld' Configured Latency: '%ld'\n\t", l->latency, l->configured_latency);
    pa_sample_spec_snprint(l_strSampleSpec, 1024, &l->sample_spec);
    pa_channel_map_snprint(l_strChannelMap, 1024, &l->channel_map);
    // printf("Proplist: '%s'\n\t", pa_proplist_to_string(l->proplist));
    printf("Sample spec: '%s'\n\t", l_strSampleSpec);
    printf("Channel map: '%s'\n\n\t", l_strChannelMap);
    if( l->active_port != NULL )
    {
       printf("Active Port Name: '%s'\n\tDescription '%s'\n\tPriority: '%d'\n\tAvailable:: '%d'\n", l->active_port->name, l->active_port->description, l->active_port->priority, l->active_port->available);
    }

    printf("\nPorts:\n");

    for(i=0; i < l->n_ports; i++) {
	printf("\t\t%d: Port Name: '%s'\n\t\tDescription '%s'\n\t\tPriority: '%d'\n\t\tAvailable:: '%d'\n", i, l->ports[i]->name, l->ports[i]->description, l->ports[i]->priority, l->ports[i]->available);
    }

    printf("\nSupported formats:\n");

    for(i=0; i < l->n_formats; i++) {
        pa_format_info_snprint(l_strFormatInfo, 1024, l->formats[i]);
	printf("\t\t%d: Supported format: '%s'\n", i, l_strFormatInfo);
    }
    printf("\n");
}

/* Anything happens call this */
static void stream_notify_cb(pa_stream *s, void *userdata) {
    char m_iSst[PA_SAMPLE_SPEC_SNPRINT_MAX];
    char m_SCmt[PA_SAMPLE_SPEC_SNPRINT_MAX];
    printf("stream_notify_cb: Using sample spec '%s', channel map '%s'.",
           pa_sample_spec_snprint(m_iSst, sizeof(m_iSst), pa_stream_get_sample_spec(s)),
           pa_channel_map_snprint(m_SCmt, sizeof(m_SCmt), pa_stream_get_channel_map(s)));

    printf("stream_notify_cb: Connected to device '%s' (index: '%u', '%s').\n",
           pa_stream_get_device_name(s),
           pa_stream_get_device_index(s),
           pa_stream_is_suspended(s) ? "" : "not");
}

/* Reques for writing length data */
static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {
    pa_usec_t usec = 0;
    int neg = 0;
    int readcount = 0;

    /* Just null readed area */
    memset(m_fSampledata, 0x00, length * 4);

    /* Read with libsndfile */
    readcount = sf_read_float(m_SInfile, m_fSampledata, length / 4);

    /* Measure latency */
    pa_stream_get_latency(s, &usec, &neg);

    /* Print some statistics */
    printf("stream_request_cb: latency %8d us request: %8ld readed %8d\r", (int)usec, length, readcount);

    /* File end if we read -1 */
    if( readcount <= 0 ) {
        m_iLoop = 1;
    }

    /* After that write to the Pulseaudio sink */
    if( pa_stream_write(s, m_fSampledata, length, NULL, 0, PA_SEEK_RELATIVE) ) {
        fprintf(stderr, "stream_request_cb: Something wrong!\n");
    }

}

/* There is not enough bytes to flow so we call underflow */
static void stream_underflow_cb(pa_stream *s, void *userdata) {
    /* We increase the latency by 50% if we get 6 m_iUnderflows and latency is under 2s
     This is very useful for over the network playback that can't handle low latencies */
    printf("stream_underflow_cb: underflow\n");
    m_iUnderflows++;

    if (m_iUnderflows >= 6 && m_iLatency < 2000000) {
        m_iLatency = (m_iLatency * 3) / 2;
        m_SBufAttr.maxlength = pa_usec_to_bytes(m_iLatency, &m_SSs);
        m_SBufAttr.tlength = pa_usec_to_bytes(m_iLatency, &m_SSs);
        pa_stream_set_buffer_attr(s, &m_SBufAttr, NULL, NULL);
        m_iUnderflows = 0;
        printf("stream_underflow_cb: latency increased to %d\n", m_iLatency);
    }
}

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
    m_iLoop = 1;
}

int main(int argc, char *argv[]) {
    pa_mainloop *l_SPaml = NULL;
    pa_mainloop_api *l_SPamlapi = NULL;
    pa_context *l_SPactx = NULL;
    pa_stream *l_SPlaystream = NULL;
    pa_operation *l_SPaop = NULL;
    int r = 0;
    int l_iPaReady = 0;
    int l_iRetval = 0;
    struct sigaction l_SSa;
    pa_channel_map l_SChannelMap;

    /* Open file. Because this is just a example we asume
      What you are doing and give file first argument */
    if (! (m_SInfile = sf_open(argv[1], SFM_READ, &m_SSfinfo))) {
        fprintf(stderr, "main: Not able to open input file %s.\n", argv[1]) ;
        sf_perror (NULL) ;
        return  1 ;
    }

    printf("main: Opened file: (%s)\n", argv[1]);

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
        return -1;
    }

    /* Create a mainloop API and connection to the default server */
    l_SPaml = pa_mainloop_new();
    l_SPamlapi = pa_mainloop_get_api(l_SPaml);

    l_SPactx = pa_context_new(l_SPamlapi, "Simple example Pulseaudio playback application");

    pa_context_connect(l_SPactx, NULL, 0, NULL);

    /* Define what callback is called in state change */
    pa_context_set_state_callback(l_SPactx, pa_state_cb, &l_iPaReady);

    /* We can't do anything until PA is ready, so just iterate the mainloop
      and continue */
    while (l_iPaReady == 0) {
        pa_mainloop_iterate(l_SPaml, 1, NULL);
    }

    if (l_iPaReady == 2) {
        l_iRetval = -1;
        goto exit;
    }

    for(r = 0; r < 1024; r++) {
      memset( &m_SSourceList[r], 0x00, sizeof(pulseinfo));
      memset( &m_SSinkList[r], 0x00, sizeof(pulseinfo));
    }
    r = 0;
    
    /* Request sink infos */
    l_SPaop = pa_context_get_sink_info_list(l_SPactx, pa_sinklist_cb, NULL);

    /* We iterate and wait for PA_OPERATION_DONE */
    while(1) {
        pa_mainloop_iterate(l_SPaml, 1, NULL);

        if (pa_operation_get_state(l_SPaop) == PA_OPERATION_DONE) {
            pa_operation_unref(l_SPaop);
            break;
        }
    }

    /* Request source infos */
    l_SPaop = pa_context_get_source_info_list(l_SPactx, pa_sourcelist_cb, NULL);

    while(1) {
        pa_mainloop_iterate(l_SPaml, 1, NULL);

        if (pa_operation_get_state(l_SPaop) == PA_OPERATION_DONE) {
            pa_operation_unref(l_SPaop);
            break;
        }
    }

    for(r = 0; r < (m_iSinkCount + 1); r++) {
      char str[512];
      printf("main: [%d] Sink name: %s (%s)\n", r, m_SSinkList[r].name, pa_channel_map_snprint(str, 512, &m_SSinkList[r].channel_map));
    }

    for(r = 0; r < (m_iSourceCount + 1); r++) {
      char str[512];
      printf("main: [%d] Source name: %s (%s)\n", r, m_SSourceList[r].name, pa_channel_map_snprint(str, 512, &m_SSourceList[r].channel_map));
    }

    r = 0;

    printf("main: We have samplerate: %5d and channels %2d\n", m_SSfinfo.samplerate, m_SSfinfo.channels);

    if( m_SSfinfo.format & SF_FORMAT_PCM_S8 ) {
        printf("main: Subformat: 8-bit!\n");

    } else if( m_SSfinfo.format & SF_FORMAT_PCM_16 ) {
        printf("main: Subformat: 16-bit!\n");

    } else if( m_SSfinfo.format & SF_FORMAT_PCM_24 ) {
        printf("main: Subformat: 24-bit!\n");

    } else if( m_SSfinfo.format & SF_FORMAT_PCM_32 ) {
        printf("main: Subformat: 32-bit!\n");

    } else if( m_SSfinfo.format & SF_FORMAT_FLOAT ) {
        printf("main: Subformat: FLOAT!\n");

    } else if( m_SSfinfo.format & SF_FORMAT_DOUBLE ) {
        printf("main: Subformat: DOUBLE!\n");
    }

    m_SSs.rate = m_SSfinfo.samplerate;
    m_SSs.channels = m_SSfinfo.channels;
    m_SSs.format = PA_SAMPLE_FLOAT32LE;

    l_SChannelMap.channels = 2;
    l_SChannelMap.map[0] = m_SSinkList[1].channel_map.map[2];
    l_SChannelMap.map[1] = m_SSinkList[1].channel_map.map[3];
 
    l_SPlaystream = pa_stream_new(l_SPactx,  "Playback", &m_SSs, &l_SChannelMap);

    if (!l_SPlaystream) {
        fprintf(stderr, "main: pa_stream_new failed\n");
        l_iRetval = -1;
        goto exit;
    }

    /* Callback for writing */
    pa_stream_set_write_callback(l_SPlaystream, stream_request_cb, m_SInfile);
    /* Callback for underflow */
    pa_stream_set_underflow_callback(l_SPlaystream, stream_underflow_cb, NULL);
    /* Stream has started */
    pa_stream_set_started_callback(l_SPlaystream, stream_notify_cb, NULL);

    m_SBufAttr.fragsize = (uint32_t) - 1;
    m_SBufAttr.maxlength = pa_usec_to_bytes(m_iLatency, &m_SSs);
    m_SBufAttr.minreq = pa_usec_to_bytes(0, &m_SSs);
    m_SBufAttr.prebuf = (uint32_t) - 1;
    m_SBufAttr.tlength = pa_usec_to_bytes(m_iLatency, &m_SSs);

    /* Connect playback to default output */
    r = pa_stream_connect_playback(l_SPlaystream, NULL, &m_SBufAttr,
                                   PA_STREAM_INTERPOLATE_TIMING
                                   | PA_STREAM_ADJUST_LATENCY
                                   | PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);

    if (r < 0) {
        printf("main: Can't connect to server. Trying with another parameters\n");
        /* Old pulse audio servers don't like the ADJUST_LATENCY flag, so retry without that */
        r = pa_stream_connect_playback(l_SPlaystream, NULL, &m_SBufAttr,
                                       PA_STREAM_INTERPOLATE_TIMING |
                                       PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
    }

    if (r < 0) {
        printf("main: pa_stream_connect_playback failed\n");
        l_iRetval = -1;
        goto exit;
    }

    /* Iterate the main m_iLoop and go again.  The second argument is whether
      or not the iteration should block until something is ready to be
      done.  Set it to zero for non-blocking. */
    while (!m_iLoop) {
        pa_mainloop_iterate(l_SPaml, 1, NULL);
    }

exit:
    /* clean up and disconnect */
    printf("\nExit and clean\n");
    sf_close(m_SInfile);
    m_SInfile = NULL;
    pa_context_disconnect(l_SPactx);
    pa_context_unref(l_SPactx);
    pa_mainloop_free(l_SPaml);
    return l_iRetval;
}


