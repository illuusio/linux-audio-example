INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIRS})

ADD_EXECUTABLE(libsndfile_sdl_play libsndfile_sdl_play.c)

TARGET_LINK_LIBRARIES(libsndfile_sdl_play ${SDL_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_sdl_play ${LIBSND_LIBRARIES})
