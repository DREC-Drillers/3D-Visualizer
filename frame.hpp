#pragma once

#include <span>

#include "renderView.hpp"
#include "window/window.hpp"

namespace visualizer {

struct Frame {
    std::span<const RenderView> views;
    FramebufferSize framebufferSize;
};

} // namespace visualizer
