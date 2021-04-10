INCLUDE_DIRECTORIES(${PORTAUDIO_INCLUDE_DIRS})

ADD_EXECUTABLE(libsndfile_port_blockplay libsndfile_port_blockplay.c)
ADD_EXECUTABLE(libsndfile_port_blockrec libsndfile_port_blockrec.c)
ADD_EXECUTABLE(libsndfile_port_play libsndfile_port_play.c)
ADD_EXECUTABLE(libsndfile_port_read libsndfile_port_read.c)
ADD_EXECUTABLE(libsndfile_port_write libsndfile_port_write.c)

TARGET_LINK_LIBRARIES(libsndfile_port_blockplay ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_blockplay ${LIBSND_LIBRARIES})

TARGET_LINK_LIBRARIES(libsndfile_port_blockrec ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_blockrec ${LIBSND_LIBRARIES})

TARGET_LINK_LIBRARIES(libsndfile_port_play ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_play ${LIBSND_LIBRARIES})

TARGET_LINK_LIBRARIES(libsndfile_port_read ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_read ${LIBSND_LIBRARIES})

TARGET_LINK_LIBRARIES(libsndfile_port_write ${PORTAUDIO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_port_write ${LIBSND_LIBRARIES})
