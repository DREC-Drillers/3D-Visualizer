#pragma once

#include <array>

#include "lightVisitor.hpp"

namespace triangles::visualizer {

class Light {
public:
    virtual ~Light() = default;

    std::array<float, 3> color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    bool enabled = true;

    virtual void accept(LightVisitor& visitor) const = 0;
};

class AmbientLight final : public Light {
public:
    void accept(LightVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

class DirectionalLight final : public Light {
public:
    // World-space direction from the surface toward the light.
    // Need not be unit length.
    std::array<float, 3> towardLight{0.4f, 0.7f, 1.0f};

    void accept(LightVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

} // namespace triangles::visualizer
