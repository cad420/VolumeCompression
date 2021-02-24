cmake_minimum_required(VERSION 3.17)
project(VoxelCompression C CXX CUDA)


find_package(CUDA REQUIRED)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CUDA_STANDARD 14)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

set(PROJECT_THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

set(NVCODEC_DIR ${PROJECT_THIRD_PARTY_DIR}/nvcodec)
set(NVCODEC_INCLUDE_DIR ${NVCODEC_DIR}/include)
set(NVCODEC_LIB_DIR ${NVCODEC_DIR}/lib)


include_directories(include)
add_subdirectory(${SRC_DIR})
add_subdirectory(test)


