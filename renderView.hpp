#pragma once

#include "core/viewport.hpp"
#include "scene/camera.hpp"
#include "scene/scene.hpp"

namespace visualizer {

struct RenderView {
    const Scene& scene;
    CameraId camera;
    NormalizedViewport viewport;
};

} // namespace visualizer
