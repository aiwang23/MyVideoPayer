cmake_minimum_required(VERSION 3.20)

project(Utils CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

include_directories(${CMAKE_SOURCE_DIR}/3rdparty/FFmpeg/include)

include_directories(.)
aux_source_directory(. SRC_LIST)

add_library(Utils
        ${SRC_LIST}
        ../PlayerCore/MyType.h
)