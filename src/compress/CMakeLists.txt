

list(
        APPEND
        VOXEL_COMPRESS_SRC
        voxel_compress.cpp
        nvenc/NvEncoder.cpp
        nvenc/CUDA/NvEncoderCuda.cpp
)

cuda_add_library(voxel_compress ${VOXEL_COMPRESS_SRC})

target_include_directories(voxel_compress PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NVCODEC_INCLUDE_DIR})

if(WIN32)
    set(NVENCODE_LIBS ${NVCODEC_LIB_DIR}/x64/nvencodeapi.lib)
else()
    set(NVENCODE_LIBS
            ${NVCODEC_LIB_DIR}/linux/stubs/x86_64/libnvcuvid-encode.so)
endif()
message(STATUS ${NVDECODE_LIBS})

target_link_libraries(voxel_compress ${NVENCODE_LIBS})

add_subdirectory(nvenc)