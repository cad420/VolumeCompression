
add_executable(BlockVolumeCompress BlockVolumeCmp.cpp)
target_include_directories(BlockVolumeCompress PUBLIC ${VolumeReader_INCLUDE_DIR})
target_link_libraries(BlockVolumeCompress PRIVATE voxel_compress)

add_executable(BlockVolumeUnCompress BlockVolumeUncmp.cpp)
target_include_directories(BlockVolumeUnCompress PUBLIC ${VolumeReader_INCLUDE_DIR})
target_link_libraries(BlockVolumeUnCompress PRIVATE voxel_uncompress)

add_executable(BlockedVolumeCmp BlockedVolumeCmp.cpp)
target_include_directories(BlockedVolumeCmp PUBLIC ${VolumeReader_INCLUDE_DIR})
target_link_libraries(BlockedVolumeCmp PRIVATE voxel_compress)

add_executable(VolumeResample VolumeResample.cpp)
target_link_libraries(VolumeResample PRIVATE TinyTIFFShared)