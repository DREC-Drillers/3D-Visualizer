cmake_minimum_required(VERSION 3.28)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use a separate build directory.")
endif()

project(Visualizer VERSION 0.1.0 LANGUAGES CXX C)

set(visualizer_build_tests_default OFF)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    set(visualizer_build_tests_default ${BUILD_TESTING})
endif()

option(VISUALIZER_BUILD_TESTS "Build visualizer tests" ${visualizer_build_tests_default})
option(VISUALIZER_BUILD_EXAMPLES "Build the interactive lighting example" OFF)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Threads REQUIRED)

include(GNUInstallDirs)

set(GLAD_SOURCE_DIR ""
    CACHE PATH "Path to generated GLAD 2 sources for OpenGL 3.3")

if(GLAD_SOURCE_DIR)
    set(visualizer_glad_source_dir "${GLAD_SOURCE_DIR}")
else()
    set(visualizer_glad_source_dir "$ENV{GLAD_SOURCE_DIR}")
endif()

if(NOT EXISTS "${visualizer_glad_source_dir}/src/gl.c" OR
   NOT EXISTS "${visualizer_glad_source_dir}/include/glad/gl.h")
    message(FATAL_ERROR
        "GLAD 2 sources not found at '${visualizer_glad_source_dir}'. "
        "Set GLAD_SOURCE_DIR to the generated GLAD source directory.")
endif()

function(visualizer_configure_target target)
    target_compile_features(${target} PRIVATE cxx_std_20)
    set_target_properties(${target} PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_SCAN_FOR_MODULES OFF)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(visualizer_glad STATIC "${visualizer_glad_source_dir}/src/gl.c")
target_compile_features(visualizer_glad PRIVATE c_std_99)
target_include_directories(visualizer_glad SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${visualizer_glad_source_dir}/include>")
set_target_properties(visualizer_glad PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    EXPORT_NAME glad)

add_library(visualizer STATIC
    backendFactory.cpp
    scene/camera.cpp
    scene/scene.cpp
    scene/lightning/lighting.cpp
    window/glfwWindow.cpp
    visualizer.cpp
    cameraController/orbitCameraController.cpp
    renderer/openGLRenderer.cpp
    renderer/openGLRenderDataPreparer.cpp
    renderer/matrices.cpp
    renderer/shaderProgram.cpp)
add_library(Visualizer::visualizer ALIAS visualizer)

target_include_directories(visualizer PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/visualizer>")
target_link_libraries(visualizer PRIVATE
    glfw visualizer_glad glm::glm Threads::Threads)
target_compile_definitions(visualizer PRIVATE GLFW_INCLUDE_NONE)
target_compile_features(visualizer PUBLIC cxx_std_20)
visualizer_configure_target(visualizer)

if(VISUALIZER_BUILD_EXAMPLES)
    add_executable(renderTest examples/renderTest.cpp)
    target_link_libraries(renderTest PRIVATE Visualizer::visualizer glfw)
    target_compile_definitions(renderTest PRIVATE GLFW_INCLUDE_NONE)
    visualizer_configure_target(renderTest)
    set_target_properties(renderTest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

if(VISUALIZER_BUILD_TESTS)
    enable_testing()

    add_executable(camera_controller_test tests/camera_controller.cpp)
    target_link_libraries(camera_controller_test PRIVATE Visualizer::visualizer)
    visualizer_configure_target(camera_controller_test)
    add_test(NAME camera_controller COMMAND camera_controller_test)

    add_executable(lighting_test tests/lighting.cpp)
    target_link_libraries(lighting_test PRIVATE Visualizer::visualizer)
    visualizer_configure_target(lighting_test)
    add_test(NAME lighting COMMAND lighting_test)

    add_executable(visualizer_consumer_test tests/consumer/main.cpp)
    target_link_libraries(visualizer_consumer_test PRIVATE Visualizer::visualizer)
    visualizer_configure_target(visualizer_consumer_test)
    add_test(NAME visualizer_consumer COMMAND visualizer_consumer_test)
endif()

include(CMakePackageConfigHelpers)

install(TARGETS visualizer visualizer_glad
    EXPORT VisualizerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
    backendFactory.hpp backendSettings.hpp frame.hpp inputState.hpp renderView.hpp
    viewport.hpp visualizer.hpp visualizerBackend.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/visualizer)
install(DIRECTORY scene cameraController window
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/visualizer
    FILES_MATCHING PATTERN "*.hpp")
install(FILES renderer/renderer.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/visualizer/renderer)
install(EXPORT VisualizerTargets
    NAMESPACE Visualizer::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Visualizer)

configure_package_config_file(cmake/VisualizerConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/VisualizerConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Visualizer)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/VisualizerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/VisualizerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VisualizerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Visualizer)
