cmake_minimum_required(VERSION 3.14)

project(VoxelCompression C CXX)
find_package(CUDA REQUIRED)

option(BUILD_TEST "whether build test" OFF)
option(BUILD_TOOL "whether build tool" ON)


find_package(CUDA REQUIRED)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CUDA_STANDARD 14)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(PROJECT_THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(NVCODEC_DIR ${PROJECT_THIRD_PARTY_DIR}/nvcodec)
set(NVCODEC_INCLUDE_DIR ${NVCODEC_DIR}/include)
set(NVCODEC_LIB_DIR ${NVCODEC_DIR}/lib)


set(LibTiff_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/third_party/libtiff/include)
if(WIN32)
set(
        LibTiff_LIBS

        ${PROJECT_SOURCE_DIR}/third_party/libtiff/lib/x64/tiff.lib

        ${PROJECT_SOURCE_DIR}/third_party/libtiff/lib/x64/tiffxx.lib
)
else()
set(
        LibTiff_LIBS

        ${PROJECT_SOURCE_DIR}/third_party/libtiff/lib/linux/libtiff.so

        ${PROJECT_SOURCE_DIR}/third_party/libtiff/lib/linux/libtiff.so
)
file(
    COPY
    "${PROJECT_SOURCE_DIR}/third_party/libtiff/lib/linux/"
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
endif()
message(STATUS ${LibTiff_LIBS})
set(FFmpeg_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/third_party/ffmpeg/include)
if(WIN32)
set(
        FFmpeg_LIBS
        ${PROJECT_SOURCE_DIR}/third_party/ffmpeg/lib/x64/avcodec.lib

        ${PROJECT_SOURCE_DIR}/third_party/ffmpeg/lib/x64/avutil.lib

)
else()
    set(
            FFmpeg_LIBS
            ${PROJECT_SOURCE_DIR}/third_party/ffmpeg/lib/linux/libavcodec.so

            ${PROJECT_SOURCE_DIR}/third_party/ffmpeg/lib/linux/libavutil.so

            ${PROJECT_SOURCE_DIR}/third_party/ffmpeg/lib/linux/libswresample.so

    )
    file(
    COPY
    "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/lib/linux/"
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()
message(!!!!!!!!)
message(STATUS ${FFmpeg_LIBS})


include_directories(include third_party)

add_subdirectory(src)

if(BUILD_TEST)
    add_subdirectory(test)
endif()

if(BUILD_TOOL)
    add_subdirectory(tools)
endif()
