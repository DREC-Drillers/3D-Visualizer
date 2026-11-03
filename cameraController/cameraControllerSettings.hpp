#pragma once

#include <numbers>

namespace visualizer {

struct OrbitCameraControllerSettings {
    float rotationSpeedRadians = 2.0f * std::numbers::pi_v<float>;
    float pitchLimitRadians = std::numbers::pi_v<float> / 2.0f - 0.01f;
    float panSpeed = 1.0f;
    float zoomSpeed = 0.15f;
    double maximumScrollDelta = 20.0;

    float initialDistance = 8.0f;
    float minimumDistance = 0.01f;
    float nearPlaneDistanceFactor = 1.1f;
    float farPlaneDistanceFactor = 0.9f;

    float minimumOrthographicSize = 0.01f;
    float maximumOrthographicSize = 10000.0f;
};

} // namespace visualizer
