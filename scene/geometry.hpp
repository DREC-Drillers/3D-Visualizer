#pragma once

#include <array>
#include <memory>
#include <utility>
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
    // When set, this immutable snapshot supplies the vertices for rendering.
    std::shared_ptr<const std::vector<Vertex>> staticVertices;

    const std::vector<Vertex>& renderVertices() const noexcept {
        return staticVertices ? *staticVertices : vertices;
    }

    void freezeVertices() {
        staticVertices =
            std::make_shared<const std::vector<Vertex>>(std::move(vertices));
        vertices.clear();
    }
};

} // namespace visualizer
