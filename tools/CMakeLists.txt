

include(${PROJECT_SOURCE_DIR}/third_party/TinyTiff.cmake)

add_executable(VolumeResample VolumeResample.cpp)
target_link_libraries(VolumeResample PRIVATE TinyTIFF)

add_executable(VolumeTifToRaw VolumeTifToRaw.cpp)
target_link_libraries(VolumeTifToRaw PRIVATE TinyTIFF)

add_executable(VolumeToH264 VolumeToH264.cpp)
target_link_libraries(VolumeToH264 PRIVATE voxel_compress)