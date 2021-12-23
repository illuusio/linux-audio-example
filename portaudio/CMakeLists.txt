INCLUDE_DIRECTORIES(${PORTAUDIO_INCLUDE_DIRS})

ADD_EXECUTABLE(libsndfile_port_blockplay libsndfile_port_blockplay.c)
ADD_EXECUTABLE(libsndfile_port_blockrec libsndfile_port_blockrec.c)
ADD_EXECUTABLE(libsndfile_port_play libsndfile_port_play.c)
ADD_EXECUTABLE(libsndfile_port_rec libsndfile_port_rec.c)

TARGET_LINK_LIBRARIES(libsndfile_port_blockplay ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_blockplay ${LIBSND_LIBRARIES})

TARGET_LINK_LIBRARIES(libsndfile_port_blockrec ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_blockrec ${LIBSND_LIBRARIES})

TARGET_LINK_LIBRARIES(libsndfile_port_play ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_play ${LIBSND_LIBRARIES})

TARGET_LINK_LIBRARIES(libsndfile_port_rec ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_rec ${LIBSND_LIBRARIES})
