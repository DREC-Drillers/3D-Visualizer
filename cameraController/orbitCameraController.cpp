#include "orbitCameraController.hpp"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <numbers>
#include <stdexcept>

namespace visualizer {
namespace {

glm::vec3 vector(const std::array<float, 3>& value) {
    return {value[0], value[1], value[2]};
}

glm::quat orientation(float yaw, float pitch) {
    return glm::angleAxis(yaw, glm::vec3(0, 1, 0)) *
           glm::angleAxis(-pitch, glm::vec3(1, 0, 0));
}

void validateSettings(const OrbitCameraControllerSettings& settings) {
    const double positiveValues[] = {settings.rotationSpeedRadians,
                                     settings.pitchLimitRadians,
                                     settings.panSpeed,
                                     settings.zoomSpeed,
                                     settings.maximumScrollDelta,
                                     settings.initialDistance,
                                     settings.minimumDistance,
                                     settings.nearPlaneDistanceFactor,
                                     settings.farPlaneDistanceFactor,
                                     settings.minimumOrthographicSize,
                                     settings.maximumOrthographicSize};

    for (double value : positiveValues) {
        if (!std::isfinite(value) || value <= 0) {
            throw std::invalid_argument(
                "Controller settings must be finite and positive");
        }
    }

    if (settings.pitchLimitRadians >= std::numbers::pi_v<float> / 2.0f ||
        settings.nearPlaneDistanceFactor <= 1 ||
        settings.farPlaneDistanceFactor >= 1 ||
        settings.minimumOrthographicSize > settings.maximumOrthographicSize) {
        throw std::invalid_argument("Invalid camera controller limits");
    }
}

bool hasInput(const InputState& input) {
    const bool dragging = (input.leftButtonDown || input.rightButtonDown) &&
                          (input.cursorDelta[0] != 0 || input.cursorDelta[1] != 0);

    return input.focused && (dragging || input.scrollDelta != 0);
}

} // namespace

OrbitCameraController::OrbitCameraController(std::array<float, 3> target,
                                             OrbitCameraControllerSettings settings)
    : settings_(settings), target_(target), distance_(settings.initialDistance) {
    validateSettings(settings_);

    for (float component : target_) {
        if (!std::isfinite(component)) {
            throw std::invalid_argument("Orbit target must be finite");
        }
    }
}

void OrbitCameraController::attach(const Camera& camera) {
    const auto offset = vector(camera.position()) - vector(target_);
    distance_ = glm::length(offset);
    if (!std::isfinite(distance_) || distance_ <= 0) {
        throw std::invalid_argument(
            "Camera position must differ from the orbit target");
    }

    yaw_ = std::atan2(offset.x, offset.z);
    pitch_ = std::asin(std::clamp(offset.y / distance_, -1.0f, 1.0f));
}

void OrbitCameraController::update(Camera& camera, const InputState& input, float,
                                   float aspect) {
    if (!hasInput(input)) {
        return;
    }

    rotate(input);
    pan(camera, input, aspect);
    zoom(camera, input.scrollDelta);
    applyTo(camera);
}

void OrbitCameraController::rotate(const InputState& input) {
    if (!input.leftButtonDown) {
        return;
    }

    const float dx = static_cast<float>(input.cursorDelta[0]);
    const float dy = static_cast<float>(input.cursorDelta[1]);
    const float fullTurn = 2.0f * std::numbers::pi_v<float>;

    yaw_ = std::remainder(yaw_ - dx * settings_.rotationSpeedRadians, fullTurn);
    pitch_ = std::clamp(pitch_ + dy * settings_.rotationSpeedRadians,
                        -settings_.pitchLimitRadians,
                        settings_.pitchLimitRadians);
}

void OrbitCameraController::pan(const Camera& camera, const InputState& input,
                                float aspect) {
    if (!input.rightButtonDown) {
        return;
    }

    const auto& settings = camera.settings();
    const float visibleHeight =
        settings.type == Camera::Type::Perspective
            ? 2.0f * distance_ * std::tan(settings.verticalFovRadians / 2.0f)
            : settings.verticalSize;
    const auto rotation = orientation(yaw_, pitch_);
    const auto right = rotation * glm::vec3(1, 0, 0);
    const auto up = rotation * glm::vec3(0, 1, 0);
    const float dx = static_cast<float>(input.cursorDelta[0]);
    const float dy = static_cast<float>(input.cursorDelta[1]);

    const auto target = vector(target_) + (-right * dx * aspect + up * dy) *
                                              visibleHeight * settings_.panSpeed;
    target_ = {target.x, target.y, target.z};
}

void OrbitCameraController::zoom(Camera& camera, double scrollDelta) {
    if (scrollDelta == 0) {
        return;
    }

    const float factor =
        std::exp(-settings_.zoomSpeed *
                 static_cast<float>(std::clamp(scrollDelta,
                                               -settings_.maximumScrollDelta,
                                               settings_.maximumScrollDelta)));
    auto settings = camera.settings();

    if (settings.type == Camera::Type::Orthographic) {
        settings.verticalSize = std::clamp(settings.verticalSize * factor,
                                           settings_.minimumOrthographicSize,
                                           settings_.maximumOrthographicSize);
        camera.setSettings(settings);
    } else {
        const float minimumDistance =
            std::max(settings_.minimumDistance,
                     settings.nearPlane * settings_.nearPlaneDistanceFactor);
        const float maximumDistance = std::max(
            minimumDistance, settings.farPlane * settings_.farPlaneDistanceFactor);
        distance_ = std::clamp(distance_ * factor, minimumDistance, maximumDistance);
    }
}

void OrbitCameraController::applyTo(Camera& camera) const {
    const auto rotation = orientation(yaw_, pitch_);
    const auto position = vector(target_) + rotation * glm::vec3(0, 0, distance_);

    camera.setPosition({position.x, position.y, position.z});
    camera.setRotation({rotation.x, rotation.y, rotation.z, rotation.w});
}

} // namespace visualizer
