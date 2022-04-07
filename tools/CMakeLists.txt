

include(${PROJECT_SOURCE_DIR}/third_party/TinyTiff.cmake)

add_executable(VolumeResample VolumeResample.cpp)
target_link_libraries(VolumeResample PRIVATE TinyTIFF)

add_executable(VolumeTifToRaw VolumeTifToRaw.cpp)
target_link_libraries(VolumeTifToRaw PRIVATE TinyTIFF)

add_executable(VolumeToH264 VolumeToH264.cpp)
target_link_libraries(VolumeToH264 PRIVATE voxel_compress ${FFmpeg_LIBS})
target_include_directories(VolumeToH264 PRIVATE ${FFmpeg_INCLUDE_DIR})

find_package(OpenMP)
add_executable(VolumeTifToH264 VolumeTifToH264.cpp)
target_link_libraries(VolumeTifToH264 PRIVATE OpenMP::OpenMP_CXX ${FFmpeg_LIBS} voxel_compress)
target_include_directories(VolumeTifToH264 PRIVATE ${FFmpeg_INCLUDE_DIR})
target_compile_features(VolumeTifToH264 PRIVATE cxx_std_17)

add_executable(ExtractBlockFromRawVolume ExtractBlockFromRawVolume.cpp)
