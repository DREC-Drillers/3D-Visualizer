#pragma once

#include <cstddef>
#include <stdexcept>

#include "../scene/lightning/light.hpp"
#include "../scene/lightning/lightVisitor.hpp"
#include "openGLLightingData.hpp"

namespace triangles::visualizer {

// Collects enabled scene lights for the OpenGL shader through Lighting::visit.
class OpenGLLightDataCollector final : public LightVisitor {
public:
    explicit OpenGLLightDataCollector(OpenGLLightingData& data) : data_(data) {}

    void visit(const AmbientLight& light) override {
        for (std::size_t i = 0; i < 3; ++i) {
            data_.ambientColor[i] += light.color[i] * light.intensity;
        }
    }

    void visit(const DirectionalLight& light) override {
        if (data_.directionalCount == data_.directional.size()) {
            throw std::length_error(
                "Renderer supports at most 16 enabled directional lights");
        }

        auto& directional = data_.directional[data_.directionalCount++];
        directional.towardLight = light.towardLight;

        for (std::size_t i = 0; i < 3; ++i) {
            directional.radiance[i] = light.color[i] * light.intensity;
        }
    }

private:
    OpenGLLightingData& data_;
};

} // namespace triangles::visualizer
