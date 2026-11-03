#pragma once

#include "scene/camera.hpp"
#include "scene/scene.hpp"
#include "viewport.hpp"

namespace visualizer {

struct RenderView {
    const Scene& scene;
    CameraId camera;
    NormalizedViewport viewport;
};

} // namespace visualizer
