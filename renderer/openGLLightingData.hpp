#pragma once

#include <array>
#include <cstddef>

namespace visualizer {

// Keep this capacity in sync with MAX_DIRECTIONAL_LIGHTS in matte.hpp.
inline constexpr std::size_t MAX_OPEN_GL_DIRECTIONAL_LIGHTS = 16;

struct OpenGLDirectionalLightData {
    std::array<float, 3> towardLight;
    std::array<float, 3> radiance;
};

// Illumination and source arrays prepared for the OpenGL matte shader.
struct OpenGLLightingData {
    std::array<float, 3> ambientColor{};
    std::array<OpenGLDirectionalLightData, MAX_OPEN_GL_DIRECTIONAL_LIGHTS>
        directional{};
    std::size_t directionalCount = 0;
};

} // namespace visualizer
