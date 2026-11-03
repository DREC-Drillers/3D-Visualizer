#pragma once

#include "../frame.hpp"

namespace visualizer {

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void render(const Frame& frame) = 0;
};

} // namespace visualizer
