#include "visualizer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

namespace triangles::visualizer {

void Viewer::addView(Scene& scene, CameraId camera, NormalizedViewport viewport,
                     std::unique_ptr<CameraController> controller) {
    scene.camera(camera);

    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) ||
        !std::isfinite(viewport.width) || !std::isfinite(viewport.height) ||
        viewport.x < 0 || viewport.y < 0 || viewport.width <= 0 ||
        viewport.height <= 0 || viewport.x + viewport.width > 1 ||
        viewport.y + viewport.height > 1) {
        throw std::invalid_argument(
            "Viewport must have positive dimensions and fit inside [0, 1]");
    }

    for (const auto& view : views_) {
        const auto& other = view.viewport;

        if (viewport.x < other.x + other.width &&
            other.x < viewport.x + viewport.width &&
            viewport.y < other.y + other.height &&
            other.y < viewport.y + viewport.height) {
            throw std::invalid_argument("Viewports must not overlap");
        }
    }

    if (controller) {
        controller->attach(scene.camera(camera));
    }

    views_.push_back({scene, camera, viewport, std::move(controller)});
}

std::optional<std::size_t> Viewer::viewAt(std::array<double, 2> position) const {
    for (std::size_t index = 0; index < views_.size(); ++index) {
        const auto& area = views_[index].viewport;
        if (position[0] >= area.x && position[0] < area.x + area.width &&
            position[1] >= area.y && position[1] < area.y + area.height) {
            return index;
        }
    }

    return std::nullopt;
}

void Viewer::update(float deltaTime) {
    if (!ViewerBackend_.window) {
        throw std::logic_error("Updating requires a window");
    }

    auto input = ViewerBackend_.window->takeInput();
    const auto size = ViewerBackend_.window->framebufferSize();
    if (!input.focused || size.width <= 0 || size.height <= 0) {
        draggedView_.reset();
        return;
    }

    if (input.leftButtonPressed || input.rightButtonPressed) {
        draggedView_ = viewAt(input.pressPosition);
    }

    const auto selectedView =
        draggedView_ ? draggedView_ : viewAt(input.cursorPosition);
    if (selectedView && views_[*selectedView].controller) {
        auto& view = views_[*selectedView];
        if (!draggedView_) {
            input.leftButtonDown = false;
            input.rightButtonDown = false;
        }

        input.cursorDelta[0] /= view.viewport.width;
        input.cursorDelta[1] /= view.viewport.height;
        const float aspect = (size.width * view.viewport.width) /
                             (size.height * view.viewport.height);

        view.controller->update(
            view.scene.camera(view.camera), input, deltaTime, aspect);
    }

    if (!input.leftButtonDown && !input.rightButtonDown) {
        draggedView_.reset();
    }
}

void Viewer::drawFrame() {
    if (!ViewerBackend_.window || !ViewerBackend_.renderer) {
        throw std::logic_error("Drawing requires a window and a renderer");
    }

    const auto size = ViewerBackend_.window->framebufferSize();
    if (size.width <= 0 || size.height <= 0) {
        return;
    }

    std::vector<RenderView> renderViews;
    renderViews.reserve(views_.size());
    for (const auto& view : views_) {
        renderViews.push_back({view.scene, view.camera, view.viewport});
    }

    const Frame frame{renderViews, size};
    ViewerBackend_.renderer->render(frame);
}

void Viewer::run() {
    if (!ViewerBackend_.window || !ViewerBackend_.renderer) {
        throw std::logic_error("Running requires a window and a renderer");
    }

    auto& window = *ViewerBackend_.window;
    auto previousTime = std::chrono::steady_clock::now();

    while (!window.shouldClose()) {
        window.pollEvents();

        if (window.shouldClose()) {
            break;
        }

        const auto currentTime = std::chrono::steady_clock::now();
        const float deltaTime =
            std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        update(std::min(deltaTime, 0.1f));
        drawFrame();

        const auto size = window.framebufferSize();
        if (size.width <= 0 || size.height <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
}

} // namespace triangles::visualizer
