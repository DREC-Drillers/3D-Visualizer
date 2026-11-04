#pragma once

#include <array>
#include <vector>

namespace visualizer {

enum class Topology { POINTS, LINES, TRIANGLES };

struct Vertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
};

struct Geometry {
    Topology topology = Topology::TRIANGLES;
    std::vector<Vertex> vertices;
};

} // namespace visualizer
