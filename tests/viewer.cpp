#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "cameraController/orbitCameraController.hpp"
#include "visualizer.hpp"

using namespace visualizer;

namespace {

class FakeWindow final : public Window {
public:
    FramebufferSize size{800, 600};

    void pollEvents() override {}
    bool shouldClose() const override { return true; }
    FramebufferSize framebufferSize() const override { return size; }
    InputState takeInput() override { return {}; }
};

class RecordingRenderer final : public Renderer {
public:
    int calls = 0;
    FramebufferSize size{};
    std::vector<CameraId> cameras;

    void render(const Frame& frame) override {
        ++calls;
        size = frame.framebufferSize;
        cameras.clear();

        for (const auto& view : frame.views) {
            cameras.push_back(view.camera);
        }
    }
};

} // namespace

TEST(Viewer, ReportsMissingBackendWithoutEnteringRunLoop) {
    Viewer viewer;

    EXPECT_THROW(viewer.update(0.01f), std::logic_error);
    EXPECT_THROW(viewer.drawFrame(), std::logic_error);
    EXPECT_THROW(viewer.run(), std::logic_error);
}

class InvalidViewportTest : public ::testing::TestWithParam<NormalizedViewport> {};

TEST_P(InvalidViewportTest, RejectsInvalidViewport) {
    Scene scene;
    const auto camera = scene.addCamera(Camera{});
    Viewer viewer;

    EXPECT_THROW(viewer.addView(scene, camera, GetParam()), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(
    Boundaries, InvalidViewportTest,
    ::testing::Values(
        NormalizedViewport{-0.1f, 0, 1, 1}, NormalizedViewport{0, -0.1f, 1, 1},
        NormalizedViewport{0, 0, 0, 1}, NormalizedViewport{0, 0, 1, -1},
        NormalizedViewport{0.5f, 0, 1, 1}, NormalizedViewport{0, 0.5f, 1, 1},
        NormalizedViewport{std::numeric_limits<float>::quiet_NaN(), 0, 1, 1},
        NormalizedViewport{0, 0, std::numeric_limits<float>::infinity(), 1}));

TEST(Viewer, RejectsUnknownCameraAndOverlappingViews) {
    Scene scene;
    Viewer viewer;
    EXPECT_THROW(viewer.addView(scene, CameraId{42}), std::out_of_range);

    const auto camera = scene.addCamera(Camera{});
    viewer.addView(scene, camera, {0, 0, 0.5f, 1});
    EXPECT_THROW(viewer.addView(scene, camera, {0.25f, 0, 0.5f, 1}),
                 std::invalid_argument);
    EXPECT_NO_THROW(viewer.addView(scene, camera, {0.5f, 0, 0.5f, 1}));
}

TEST(Viewer, PassesViewsAndFramebufferToRendererAndSkipsMinimizedFrames) {
    auto window = std::make_unique<FakeWindow>();
    auto renderer = std::make_unique<RecordingRenderer>();
    auto* windowState = window.get();
    auto* rendered = renderer.get();
    Viewer viewer({std::move(window), std::move(renderer)});
    Scene scene;
    const auto left = scene.addCamera(Camera{});
    const auto right = scene.addCamera(Camera{});
    viewer.addView(scene, left, {0, 0, 0.5f, 1});
    viewer.addView(scene, right, {0.5f, 0, 0.5f, 1});

    viewer.drawFrame();

    EXPECT_EQ(rendered->calls, 1);
    EXPECT_EQ(rendered->size.width, 800);
    EXPECT_EQ(rendered->size.height, 600);
    EXPECT_EQ(rendered->cameras, (std::vector<CameraId>{left, right}));

    windowState->size = {0, 0};
    viewer.drawFrame();
    EXPECT_EQ(rendered->calls, 1);
}

TEST(OrbitCameraController, RejectsCameraAtOrbitTargetAndInvalidSettings) {
    Camera camera;
    OrbitCameraController controller;
    EXPECT_THROW(controller.attach(camera), std::invalid_argument);

    OrbitCameraControllerSettings settings;
    settings.zoomSpeed = 0;
    EXPECT_THROW((OrbitCameraController({}, settings)), std::invalid_argument);

    EXPECT_THROW(
        (OrbitCameraController({0, std::numeric_limits<float>::infinity(), 0})),
        std::invalid_argument);
}
