

list(
        APPEND
        VOXEL_COMPRESS_SRC
        voxel_compress.cpp
        nvenc/NvEncoder.cpp
)

add_library(voxel_compress ${VOXEL_COMPRESS_SRC})

target_include_directories(voxel_compress PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NVCODEC_INCLUDE_DIR})



add_subdirectory(nvenc)