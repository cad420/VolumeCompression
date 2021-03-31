
add_executable(test_compress test_compress.cpp)
target_link_libraries(test_compress PUBLIC
        voxel_compress)

add_executable(test_uncompress test_uncompress.cpp)
target_link_libraries(test_uncompress PUBLIC voxel_uncompress voxel_compress)



add_executable(test_struct_size test_struct_size.cpp)