#pragma once

#include <memory>
#include <utility>

#include "../renderer/renderer.hpp"
#include "../window/window.hpp"

namespace visualizer {

struct ViewerBackend {
    ViewerBackend() = default;

    ViewerBackend(std::unique_ptr<Window> window, std::unique_ptr<Renderer> renderer)
        : window(std::move(window)), renderer(std::move(renderer)) {}

    ViewerBackend(const ViewerBackend&) = delete;
    ViewerBackend& operator=(const ViewerBackend&) = delete;

    ViewerBackend(ViewerBackend&&) noexcept = default;

    ViewerBackend& operator=(ViewerBackend&& other) noexcept {
        if (this != &other) {
            // The old renderer may need its window to release resources.
            renderer.reset();
            window = std::move(other.window);
            renderer = std::move(other.renderer);
        }

        return *this;
    }

    ~ViewerBackend() = default;

    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
};

} // namespace visualizer
