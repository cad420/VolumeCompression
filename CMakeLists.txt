cmake_minimum_required(VERSION 3.17)

project(VoxelCompression C CXX CUDA)

option(BUILD_TEST "whether build test" ON)
option(BUILD_TOOL "whether build tool" ON)


find_package(CUDA REQUIRED)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CUDA_STANDARD 14)

set(PROJECT_THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(NVCODEC_DIR ${PROJECT_THIRD_PARTY_DIR}/nvcodec)
set(NVCODEC_INCLUDE_DIR ${NVCODEC_DIR}/include)
set(NVCODEC_LIB_DIR ${NVCODEC_DIR}/lib)


include_directories(include third_party)

include(third_party/TinyTiff.cmake)

include(third_party/VolumeReader.cmake)

add_subdirectory(src)

if(BUILD_TEST)
    add_subdirectory(test)
endif()

if(BUILD_TOOL)
    add_subdirectory(tools)
endif()
