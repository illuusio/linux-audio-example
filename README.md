linux-audio-example
===================

Some audio examples using with Linux

libsndfile_pulse
================
Pulseaudio is de-facto standard of Linux audio server. This is small example how to use it without threaded-mainloop.

It's constructed from

* http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncDeviceList/
* http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncPlayback/

and added libsndfile[http://www.mega-nerd.com/libsndfile/] support

libsndfile_pulse compile
========================
To compile file use

<pre>
gcc -g $(pkg-config --cflags --libs libpulse) -lm -lsndfile libsndfile_pulse.c -o libsndfile_pulse
</pre>

libsndfile_pulse run
========================
run with
./libsndfile_pulse audio.wav/.flac/.aiff (all what libsndfile supports)

libsndfile_port
================
Portaudio is highend small audio abstraction library with sane license. This example shows how to use it with libsndfile

It's constructed from
https://www.assembla.com/code/portaudio/subversion/nodes/1922/portaudio/trunk/examples/paex_sine.c

and added libsndfile[http://www.mega-nerd.com/libsndfile/] support

libsndfile_port compile
========================
To compile file use

<pre>
gcc -g $(pkg-config --cflags --libs portaudio-2.0) -lm -lsndfile libsndfile_port.c -o libsndfile_port
</pre>

libsndfile_port run
========================
run with
./libsndfile_port audio.wav/.flac/.aiff (all what libsndfile supports)


