include(FetchContent)

# Download pybind dependency when pybind11 is missing
if (NOT TARGET pybind11)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG 97784dad3e518ccb415d5db57ff9b933495d9024
    )
    FetchContent_MakeAvailable(pybind11)
    # add_subdirectory(${pybind11_SOURCE_DIR} pybind11_bin)
endif()

pybind11_add_module(_offscreen_renderer offscreen_renderer.cc)
set_target_properties(_offscreen_renderer PROPERTIES LIBRARY_OUTPUT_DIRECTORY $<1:${PROJECT_SOURCE_DIR}/python>)
target_link_libraries(_offscreen_renderer PUBLIC offscreen_renderer)
