cmake_minimum_required(VERSION 3.20)

project(PlayerCore CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

include_directories(
        ${CMAKE_SOURCE_DIR}/3rdparty/FFmpeg/include
        "${CMAKE_SOURCE_DIR}/3rdparty/SDL2/include"
)
include_directories(../Utils)

include_directories(.)
aux_source_directory(. SRC_LIST)

add_library(PlayerCore
        ${SRC_LIST}
        VideoPlayerCallBack.h
)
