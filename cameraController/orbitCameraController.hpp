#pragma once

#include <array>

#include "cameraController.hpp"
#include "cameraControllerSettings.hpp"

namespace visualizer {

class OrbitCameraController final : public CameraController {
public:
    explicit OrbitCameraController(std::array<float, 3> target = {},
                                   OrbitCameraControllerSettings settings = {});

    void attach(const Camera& camera) override;
    void update(Camera& camera, const InputState& input, float deltaTime,
                float aspect) override;

private:
    void rotate(const InputState& input);
    void pan(const Camera& camera, const InputState& input, float aspect);
    void zoom(Camera& camera, double scrollDelta);
    void applyTo(Camera& camera) const;

    OrbitCameraControllerSettings settings_;
    std::array<float, 3> target_;
    float distance_;
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
};

} // namespace visualizer
