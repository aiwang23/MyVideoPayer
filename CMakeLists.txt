cmake_minimum_required(VERSION 3.20)

add_subdirectory(Sources)

project(Client CXX)

# 链接 FFmpeg SDL2 Lib
target_link_libraries(Window
        # FFmpeg
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale

        # SDL2
#        mingw32
        SDL2main
        SDL2

        PlayerCore
)

