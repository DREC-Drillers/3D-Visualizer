#pragma once

#include <array>

namespace triangles::visualizer {

struct Material {
    std::array<float, 4> color{0.7f, 0.7f, 0.7f, 1.0f};

    // Points and lines are always drawn without lighting.
    bool lit = true;
};

}
