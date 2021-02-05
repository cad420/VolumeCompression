find_package(CUDA REQUIRED)
message(STATUS ${CUDA_INCLUDE_DIRS})
target_include_directories(voxel_compress PRIVATE ${CUDA_INCLUDE_DIRS})
target_link_directories(voxel_compress PRIVATE ${CUDA_LIBRARIES})
set_target_properties(
        voxel_compress
        PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_ARCHITECTURES 75
)