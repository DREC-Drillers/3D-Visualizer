#pragma once

#include <cstdint>
#include <string>

namespace visualizer {

enum class GraphicsApi { OPEN_GL, VULKAN };

enum class WindowLibrary { GLFW };

struct WindowSettings {
    std::string title = "Visualizer";
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool resizable = true;
    bool visible = true;
};

struct BackendSettings {
    GraphicsApi graphics = GraphicsApi::OPEN_GL;
    WindowLibrary windowLibrary = WindowLibrary::GLFW;
    WindowSettings window;
};

} // namespace visualizer
