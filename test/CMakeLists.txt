
add_executable(test_compress test_compress.cpp)
target_link_libraries(test_compress PUBLIC
        voxel_compress)

add_executable(test_uncompress test_uncompress.cpp)
target_link_libraries(test_uncompress PUBLIC voxel_uncompress voxel_compress)



add_executable(test_struct_size test_struct_size.cpp)

add_executable(test_json test_json.cpp)
target_link_libraries(test_json PRIVATE TinyTIFF)
message(STATUS ${CMAKE_BINARY_DIR}/_deps/tinytiff-src/src)
#target_include_directories(test_json PRIVATE ${CMAKE_BINARY_DIR}/_deps/tinytiff-src/src)

add_executable(test_raw2tif test_raw2tif.cpp)
target_link_libraries(test_raw2tif PRIVATE TinyTIFF)

add_executable(test_tif2raw test_tif2raw.cpp)
target_link_libraries(test_tif2raw PRIVATE TinyTIFF)

add_executable(test_raw_diff test_raw_diff.cpp)

add_executable(test_read_tif_from_raw test_read_tif_from_raw.cpp)
target_link_libraries(test_read_tif_from_raw PRIVATE TinyTIFF)

add_executable(test_merge_tif test_merge_tif.cpp)
target_link_libraries(test_merge_tif PRIVATE TinyTIFF)