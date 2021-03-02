
add_executable(test_compress test_opts.cpp)
target_link_libraries(test_compress PUBLIC
        voxel_compress)

add_executable(test_uncompress test_uncompress.cpp)
target_link_libraries(test_uncompress PUBLIC voxel_uncompress voxel_compress)



add_executable(test_cuda_gl test_cuda_gl.cpp ${PROJECT_THIRD_PARTY_DIR}/glad/src/glad.c)
target_include_directories(test_cuda_gl PUBLIC ${PROJECT_THIRD_PARTY_DIR}/glad/include
        ${PROJECT_THIRD_PARTY_DIR}/glfw/include ${GLM_INCLUDE_DIR})
find_package(OpenGL)
if(OPENGL_FOUND)
    message(STATUS ${OPENGL_INCLUDE_DIR})
    message(STATUS ${OPENGL_LIBRARIES})
endif()
target_link_libraries(test_cuda_gl PUBLIC voxel_uncompress voxel_compress ${CMAKE_DL_LIBS} glfw)

add_executable(test_struct_size test_struct_size.cpp)