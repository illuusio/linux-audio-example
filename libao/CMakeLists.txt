INCLUDE_DIRECTORIES(${LIBAO_INCLUDE_DIRS})

ADD_EXECUTABLE(libsndfile_libao_blockplay libsndfile_libao_blockplay.c)

TARGET_LINK_LIBRARIES(libsndfile_libao_blockplay ${LIBAO_LIBRARIES})
TARGET_LINK_LIBRARIES(libsndfile_libao_blockplay ${LIBSND_LIBRARIES})
