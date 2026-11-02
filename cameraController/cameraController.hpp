#pragma once

#include "../inputState.hpp"
#include "../scene/camera.hpp"

namespace triangles::visualizer {

class CameraController {
public:
    virtual ~CameraController() = default;

    virtual void attach(const Camera& camera) = 0;
    virtual void update(Camera& camera, const InputState& input, float deltaTime,
                        float aspect) = 0;
};

} // namespace triangles::visualizer
