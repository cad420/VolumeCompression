list(
        APPEND
        VOXEL_UNCOMPRESS_SRC
        voxel_uncompress.cpp
        nvdec/NvDecoder.cpp
)
cuda_add_library(voxel_uncompress ${VOXEL_UNCOMPRESS_SRC})

target_include_directories(voxel_uncompress PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NVCODEC_INCLUDE_DIR}
        ${CUDA_INCLUDE_DIRS})
set(NVDECODE_LIBS ${NVCODEC_LIB_DIR}/x64/nvcuvid.lib)
target_link_libraries(voxel_uncompress ${CUDA_CUDA_LIBRARY} ${NVDECODE_LIBS})
set_target_properties(
        voxel_uncompress
        PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_ARCHITECTURES 75
)
