cmake_minimum_required(VERSION 3.1)
project(MeshFEM)

# CMP0063: Honor visibility properties for all target types.
if(POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Configurable options
option(USE_OSMESA "Use the software rasterization library OSMesa instead of a GPU-accelerated EGL/CGL context" OFF)

# Color diagnostics
add_definitions(-fdiagnostics-color=always)

# Export compile flags (used for autocompletion of the C++ code)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# We need -fPIC when compiling our libraries and our dependencies for
# the python bindings (shared libraries) to link.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(offscreen_renderer INTERFACE)

find_package(Eigen3 REQUIRED)
target_include_directories(offscreen_renderer INTERFACE ${EIGEN3_INCLUDE_DIR})


find_package(GLEW REQUIRED)
target_link_libraries(offscreen_renderer INTERFACE GLEW::GLEW ${OPENGL_LIBRARIES})
target_compile_definitions(offscreen_renderer INTERFACE -DGLEW_STATIC)
target_compile_options(offscreen_renderer INTERFACE -Wno-deprecated-declarations)


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

set(SHADER_PATH  ${CMAKE_SOURCE_DIR} CACHE PATH "location of shader programs")
add_definitions("-DSHADER_PATH=\"${SHADER_PATH}\"")

add_executable(demo demo.cc)
target_compile_definitions(demo PRIVATE "-DSHADER_PATH=\"${SHADER_PATH}\"")
target_link_libraries(demo offscreen_renderer)
