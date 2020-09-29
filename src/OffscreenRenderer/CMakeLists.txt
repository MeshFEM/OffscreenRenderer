add_library(offscreen_renderer INTERFACE)
target_include_directories(offscreen_renderer INTERFACE ..)

find_package(Eigen3 REQUIRED)
target_include_directories(offscreen_renderer INTERFACE ${EIGEN3_INCLUDE_DIR})

find_package(GLEW REQUIRED)
target_link_libraries(offscreen_renderer INTERFACE GLEW::GLEW ${OPENGL_LIBRARIES})
target_compile_options(offscreen_renderer INTERFACE -Wno-deprecated-declarations -march=native -ffast-math)

find_package(PNG REQUIRED)
target_link_libraries(offscreen_renderer INTERFACE PNG::PNG)

if (USE_OSMESA)
    find_package(OSMesa REQUIRED)
    message("WARNING: GLEW and OSMESA from Ubuntu seem incompatible (glewInit fails) with a 'Missing GL Version' error")
    target_compile_definitions(offscreen_renderer INTERFACE -DUSE_OSMESA)
    target_link_libraries(offscreen_renderer INTERFACE OSMesa::OSMesa)
else()
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        # Default to CGL on Linux
        find_package(OpenGL REQUIRED)
        target_compile_definitions(offscreen_renderer INTERFACE -DUSE_CGL)
    else()
        set(OpenGL_GL_PREFERENCE "LEGACY")
        find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
        # Default to EGL on Linux
        target_compile_definitions(offscreen_renderer INTERFACE -DUSE_EGL)
        target_link_libraries(offscreen_renderer INTERFACE OpenGL::EGL)
    endif()
endif()

set(SHADER_PATH  ${CMAKE_SOURCE_DIR}/shaders CACHE PATH "location of shader programs")
add_definitions("-DSHADER_PATH=\"${SHADER_PATH}\"")

add_executable(demo demo.cc)
target_compile_definitions(demo PRIVATE "-DSHADER_PATH=\"${SHADER_PATH}\"")
target_link_libraries(demo offscreen_renderer)
