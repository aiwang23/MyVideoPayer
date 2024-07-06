cmake_minimum_required(VERSION 3.20)
project(Window CXX C)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 修改 .EXE 文件生成路径 到 bin/
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/../bin")

# 搜索 Qt6 Lib
set(CMAKE_PREFIX_PATH "C:/Qt/6.4.3/msvc2019_64/lib/cmake")
find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        OpenGL
        OpenGLWidgets
        REQUIRED
)

# FFmpeg SDL2
include_directories(Window
        "${CMAKE_SOURCE_DIR}/3rdparty/FFmpeg/include"
        "${CMAKE_SOURCE_DIR}/3rdparty/SDL2/include"
)
link_directories(Window
        "${CMAKE_SOURCE_DIR}/3rdparty/FFmpeg/lib"
        "${CMAKE_SOURCE_DIR}/3rdparty/SDL2/lib/x64"
        "${CMAKE_SOURCE_DIR}/lib"
)


# 自动搜索 *.h  *.c  *.cpp  *.ui 代码
include_directories(
        .
        ../PlayerCore
        ../Utils
)

aux_source_directory(. SRC_LIST)

# exe文件
add_executable(Window WIN32
        ${SRC_LIST}
        displaywindow.ui
        FunctionTransfer.cpp
        FunctionTransfer.h
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        setvideourldialog.cpp
        setvideourldialog.h
        setvideourldialog.ui
        videoslider.cpp
        videoslider.h
)

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
        Utils
)

# 把 FFmpeg SDL2 的 dll 拷到 .exe的目录bin中
file(COPY "${CMAKE_SOURCE_DIR}/3rdparty/FFmpeg/bin/" DESTINATION "${CMAKE_BINARY_DIR}/../bin/")
file(COPY "${CMAKE_SOURCE_DIR}/3rdparty/SDL2/bin/" DESTINATION "${CMAKE_BINARY_DIR}/../bin/")


# 链接 Qt6 Lib
target_link_libraries(Window
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
)
if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    # 拷贝 Qt6 DLL
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
