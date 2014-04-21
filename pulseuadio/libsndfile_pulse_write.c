/*
 * Most of the code is taken from these tutorials
 * http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncDeviceList/
 * http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncPlayback/
 * And added audio recording with libsndfile (WAV)
 *
 * You need:
 * Pulseaudio development file (headers and libraries) http://www.freedesktop.org/wiki/Software/PulseAudio/ at least version 3.0
 * Libsnfile development file (headers and libraries) http://www.mega-nerd.com/libsndfile/ at least version 1.0.25
 *
 * Compile with
 * gcc -g $(pkg-config --cflags --libs libpulse) -lm -lsndfile libsndfile_pulse_write.c -std=c99 -Wall -o libsndfile_pulse_write
 *
 * Run with ./libsndfile_pulse_write some.wav (Warning! Will overwrite without warning!)
 */

#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <sndfile.h>

static int latency = 20000; /* start latency in micro seconds */
const void *sampledata;
static pa_buffer_attr bufattr;
static int underflows = 0;
static pa_sample_spec ss;
SNDFILE *outfile = NULL;
SF_INFO sfinfo;
int loop = 0;

/* When context change state this called */
void pa_state_cb(pa_context *c, void *userdata) {
    pa_context_state_t state;
    int *pa_ready = userdata;
    state = pa_context_get_state(c);

    switch  (state) {
            /* These are just here for reference */
        case PA_CONTEXT_UNCONNECTED:
            printf("PA_CONTEXT_UNCONNECTED\n");
            break;

        case PA_CONTEXT_CONNECTING:
            printf("PA_CONTEXT_CONNECTING\n");
            break;

        case PA_CONTEXT_AUTHORIZING:
            printf("PA_CONTEXT_AUTHORIZING\n");
            break;

        case PA_CONTEXT_SETTING_NAME:
            printf("PA_CONTEXT_NAME\n");
            break;

        default:
            break;

        case PA_CONTEXT_FAILED:
            printf("PA_CONTEXT_FAILED\n");
            *pa_ready = 2;
            break;

        case PA_CONTEXT_TERMINATED:
            printf("PA_CONTEXT_TERMINATED\n");
            *pa_ready = 2;
            break;

        case PA_CONTEXT_READY:
            printf("PA_CONTEXT_READY\n");

            *pa_ready = 1;
            break;
    }
}

/* List sinks in our system (For playback) */
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata) {
    if( eol ) {
        return;
    }

    printf("Sink Name: %s ", l->name);
    printf("Description: %s\n", l->description);
}

/* List sources in our system (For recording) */
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata) {
    if( eol ) {
        return;
    }

    printf("Source Name: %s ", l->name);
    printf("Description: %s\n", l->description);
}

/* Anything happens call this */
static void stream_notify_cb(pa_stream *s, void *userdata) {
    char sst[PA_SAMPLE_SPEC_SNPRINT_MAX];
    char cmt[PA_SAMPLE_SPEC_SNPRINT_MAX];
    printf("Using sample spec '%s', channel map '%s'.",
           pa_sample_spec_snprint(sst, sizeof(sst), pa_stream_get_sample_spec(s)),
           pa_channel_map_snprint(cmt, sizeof(cmt), pa_stream_get_channel_map(s)));

    printf("Connected to device %s (index: %u, %ssuspended).\n",
           pa_stream_get_device_name(s),
           pa_stream_get_device_index(s),
           pa_stream_is_suspended(s) ? "" : "not ");
}

/* Reques for writing length data */
static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {
    pa_usec_t usec = 0;
    int neg = 0;
    int writecount = 0;
    size_t readed;
    /* In future is this is needed enable
       SNDFILE *sndfile = (SNDFILE *)userdata;
     */

    /* idea is like this:
           1# You peek datas pointer
           2# Yoy get how much data there is (it should be as much length is
           3# After you have done what you want you drop package (There is no pointer anymore after drop)
    */
    if (pa_stream_peek(s, &sampledata, &readed) < 0) {
        fprintf(stderr, "Reading from device failed!");
        loop = 1;
        return;
    }

    /* Read with libsndfile */
    writecount = sf_write_float(outfile, sampledata, readed / 4);

    pa_stream_drop(s);

    sampledata = NULL;

    /* Measure latency */
    pa_stream_get_latency(s, &usec, &neg);

    /* Print some statistics */
    printf("latency %8d us request: %8ld/%8ld readed %8d\r", (int)usec, length, readed, writecount);

    pa_stream_drop(s);
}

/* There is not enough bytes to flow so we call underflow */
static void stream_underflow_cb(pa_stream *s, void *userdata) {
    /* We increase the latency by 50% if we get 6 underflows and latency is under 2s
     This is very useful for over the network record that can't handle low latencies */
    printf("underflow\n");
    underflows++;

    if (underflows >= 6 && latency < 2000000) {
        latency = (latency * 3) / 2;
        bufattr.maxlength = pa_usec_to_bytes(latency, &ss);
        bufattr.tlength = pa_usec_to_bytes(latency, &ss);
        pa_stream_set_buffer_attr(s, &bufattr, NULL, NULL);
        underflows = 0;
        printf("latency increased to %d\n", latency);
    }
}

/* Handle termination with CTRL-C */
static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
    loop = 1;
}

int main(int argc, char *argv[]) {
    pa_mainloop *pa_ml = NULL;
    pa_mainloop_api *pa_mlapi = NULL;
    pa_context *pa_ctx = NULL;
    pa_stream *recordstream = NULL;
    pa_operation *pa_op = NULL;
    int r = 0;
    int pa_ready = 0;
    int retval = 0;
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
        printf ("Not able to open output file %s.\n", argv[1]) ;
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

    /* Create a mainloop API and connection to the default server */
    pa_ml = pa_mainloop_new();
    pa_mlapi = pa_mainloop_get_api(pa_ml);
    pa_ctx = pa_context_new(pa_mlapi, "Simple Pulseaudio record application");
    pa_context_connect(pa_ctx, NULL, 0, NULL);

    /* Define what callback is called in state change */
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

    /* We can't do anything until PA is ready, so just iterate the mainloop
      and continue */
    while (pa_ready == 0) {
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }

    if (pa_ready == 2) {
        retval = -1;
        goto exit;
    }

    /* Request sink infos */
    pa_op = pa_context_get_sink_info_list(pa_ctx, pa_sinklist_cb, NULL);

    /* We iterate and wait for PA_OPERATION_DONE */
    while(1) {
        pa_mainloop_iterate(pa_ml, 1, NULL);

        if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
            pa_operation_unref(pa_op);
            break;
        }
    }

    /* Request source infos */
    pa_op = pa_context_get_source_info_list(pa_ctx, pa_sourcelist_cb, NULL);

    while(1) {
        pa_mainloop_iterate(pa_ml, 1, NULL);

        if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
            pa_operation_unref(pa_op);
            break;
        }
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

    ss.rate = sfinfo.samplerate;
    ss.channels = sfinfo.channels;
    ss.format = PA_SAMPLE_FLOAT32LE;

    recordstream = pa_stream_new(pa_ctx, "Record", &ss, NULL);

    if (!recordstream) {
        printf("pa_stream_new failed\n");
    }

    /* Callback for writing */
    pa_stream_set_read_callback(recordstream, stream_request_cb, outfile);
    /* Callback for underflow */
    pa_stream_set_underflow_callback(recordstream, stream_underflow_cb, NULL);
    /* Stream has started */
    pa_stream_set_started_callback(recordstream, stream_notify_cb, NULL);

    bufattr.fragsize = (uint32_t) - 1;
    bufattr.maxlength = pa_usec_to_bytes(latency, &ss);
    bufattr.minreq = pa_usec_to_bytes(0, &ss);
    bufattr.prebuf = (uint32_t) - 1;
    bufattr.tlength = pa_usec_to_bytes(latency, &ss);

    /* Connect record to default input */
    r = pa_stream_connect_record(recordstream, NULL, &bufattr,
                                 PA_STREAM_INTERPOLATE_TIMING
                                 | PA_STREAM_ADJUST_LATENCY
                                 | PA_STREAM_AUTO_TIMING_UPDATE);

    if (r < 0) {
        /* Old pulse audio servers don't like the ADJUST_LATENCY flag, so retry without that */
        r = pa_stream_connect_record(recordstream, NULL, &bufattr,
                                     PA_STREAM_INTERPOLATE_TIMING |
                                     PA_STREAM_AUTO_TIMING_UPDATE);
    }

    if (r < 0) {
        printf("pa_stream_connect_record failed\n");
        retval = -1;
        goto exit;
    }

    /* Iterate the main loop and go again.  The second argument is whether
      or not the iteration should block until something is ready to be
      done.  Set it to zero for non-blocking. */
    while (!loop) {
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }

exit:
    /* clean up and disconnect */
    printf("\nExit and clean\n");
    sf_close(outfile);
    outfile = NULL;
    pa_context_disconnect(pa_ctx);
    pa_context_unref(pa_ctx);
    pa_mainloop_free(pa_ml);
    return retval;
}


