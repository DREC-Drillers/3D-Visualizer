#include "openGLRenderDataPreparer.hpp"

#include <stdexcept>

namespace visualizer {

GLenum primitiveTopology(Topology topology) {
    switch (topology) {
        case Topology::POINTS:
            return GL_POINTS;
        case Topology::LINES:
            return GL_LINES;
        case Topology::TRIANGLES:
            return GL_TRIANGLES;
        default:
            throw std::invalid_argument("Unknown geometry topology");
    }
}

} // namespace visualizer
