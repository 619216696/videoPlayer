cmake_minimum_required(VERSION 3.16)

project(videoPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Multimedia)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appvideoPlayer
    main.cpp
)

qt_add_qml_module(appvideoPlayer
    URI videoPlayer
    VERSION 1.0
    QML_FILES Main.qml
    SOURCES videoplayer.h videoplayer.cpp
    SOURCES videoDecoder.h videoDecoder.cpp
    SOURCES decoderBase.h
    SOURCES audioDecoder.h audioDecoder.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appvideoPlayer PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appvideoPlayer
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appvideoPlayer
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Multimedia
)

include(GNUInstallDirs)
install(TARGETS appvideoPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 添加ffmpeg库的目录
set(FFMPEG_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib")
set(FFMPEG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/include")

# 添加ffmpeg的头文件目录
include_directories(${FFMPEG_INCLUDE_DIR})

# 查找并链接所有的ffmpeg库
foreach(LIBRARY_NAME avcodec avdevice avfilter avformat avutil postproc swresample swscale)
    find_library(${LIBRARY_NAME}_PATH NAMES ${LIBRARY_NAME} PATHS ${FFMPEG_LIB_DIR} NO_DEFAULT_PATH)
    if(${LIBRARY_NAME}_PATH)
        target_link_libraries(appvideoPlayer
            PRIVATE ${${LIBRARY_NAME}_PATH}
        )
    else()
        message(FATAL_ERROR "${LIBRARY_NAME} not found")
    endif()
endforeach()

# 确保在构建时能找到ffmpeg库
link_directories(${FFMPEG_LIB_DIR})

# 确定动态库文件目录相对于CMakeLists.txt的位置
set(FFMPEG_LIBRARIES_DIR "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/dll")

# 定义需要拷贝的动态库文件列表
set(FFMPEG_LIBRARY_FILES
    # 在这里列出你需要拷贝的具体动态库文件名，例如：
    ${FFMPEG_LIBRARIES_DIR}/avcodec-61.dll
    ${FFMPEG_LIBRARIES_DIR}/avdevice-61.dll
    ${FFMPEG_LIBRARIES_DIR}/avfilter-10.dll
    ${FFMPEG_LIBRARIES_DIR}/avformat-61.dll
    ${FFMPEG_LIBRARIES_DIR}/avutil-59.dll
    ${FFMPEG_LIBRARIES_DIR}/postproc-58.dll
    ${FFMPEG_LIBRARIES_DIR}/swresample-5.dll
    ${FFMPEG_LIBRARIES_DIR}/swscale-8.dll
)

foreach(LIBRARY_FILE ${FFMPEG_LIBRARY_FILES})
    get_filename_component(LIBRARY_BASENAME ${LIBRARY_FILE} NAME)
    add_custom_command(
        TARGET appvideoPlayer  # 替换为你的可执行文件的目标名称
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LIBRARY_FILE}"
                $<TARGET_FILE_DIR:appvideoPlayer>
    )
endforeach()
