PROJECT(linux-audio-example)
CMAKE_MINIMUM_REQUIRED(VERSION 3.19)
FIND_PACKAGE(PkgConfig)

# If you don't want to build something just comment out that line
PKG_CHECK_MODULES(LIBSND REQUIRED sndfile)
PKG_CHECK_MODULES(LIBAO REQUIRED ao)
PKG_CHECK_MODULES(PULSEAUDIO REQUIRED libpulse libpulse-simple)
PKG_CHECK_MODULES(PORTAUDIO REQUIRED portaudio-2.0)
PKG_CHECK_MODULES(SDL REQUIRED sdl2)

# If you commented out something above. Comment out ADD_SUBDIRECTORY also
ADD_SUBDIRECTORY(libao)
ADD_SUBDIRECTORY(pulseaudio)
ADD_SUBDIRECTORY(portaudio)
ADD_SUBDIRECTORY(sdl)
