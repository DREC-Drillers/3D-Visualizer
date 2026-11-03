cmake_minimum_required(VERSION 3.28)
project(VisualizerConsumer LANGUAGES CXX)

if(VISUALIZER_SOURCE_DIR)
    set(VISUALIZER_BUILD_TESTS OFF)
    set(VISUALIZER_BUILD_EXAMPLES OFF)
    add_subdirectory("${VISUALIZER_SOURCE_DIR}" visualizer)
else()
    find_package(Visualizer 0.1 CONFIG REQUIRED)
endif()

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE Visualizer::visualizer)
set_property(TARGET consumer PROPERTY CXX_SCAN_FOR_MODULES OFF)
