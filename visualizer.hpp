#pragma once

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "cameraController/cameraController.hpp"
#include "renderView.hpp"
#include "visualizerBackend.hpp"

namespace visualizer {

class Viewer {
private:
    struct ControlledView {
        Scene& scene;
        CameraId camera;
        NormalizedViewport viewport;
        std::unique_ptr<CameraController> controller;
    };

    ViewerBackend ViewerBackend_;
    std::vector<ControlledView> views_;
    std::optional<std::size_t> draggedView_;

    std::optional<std::size_t> viewAt(std::array<double, 2> position) const;

public:
    Viewer() = default;
    explicit Viewer(ViewerBackend backend) : ViewerBackend_(std::move(backend)) {}

    void addView(Scene& scene, CameraId camera, NormalizedViewport viewport = {},
                 std::unique_ptr<CameraController> controller = nullptr);

    void addBackend(ViewerBackend backend) {
        ViewerBackend_.renderer.reset();
        ViewerBackend_ = std::move(backend);
        draggedView_.reset();
    }

    void run();
    void update(float deltaTime);
    void drawFrame();
};

} // namespace visualizer
