#include <cmath>
#include <memory>
#include <stdexcept>

#include <gtest/gtest.h>

#include "cameraController/orbitCameraController.hpp"
#include "core/visualizer.hpp"

using namespace visualizer;

class TestWindow final : public Window {
public:
    InputState input;
    FramebufferSize size{1000, 1000};

    void pollEvents() override {}
    bool shouldClose() const override { return false; }
    FramebufferSize framebufferSize() const override { return size; }
    InputState takeInput() override { return input; }
};

class RecordingController final : public CameraController {
public:
    int calls = 0;
    InputState lastInput;
    float lastAspect = 0;

    void attach(const Camera&) override {}
    void update(Camera&, const InputState& input, float, float aspect) override {
        ++calls;
        lastInput = input;
        lastAspect = aspect;
    }
};

TEST(OrbitCameraController, HandlesInputSequenceAndFocus) {
    Camera camera;
    camera.setPosition({0, 0, 8});
    OrbitCameraController controller;
    controller.attach(camera);
    controller.update(camera, {}, 0.016f, 1);
    EXPECT_NEAR(camera.position()[2], 8, 0.0001f);

    InputState input;
    input.leftButtonDown = true;
    input.cursorDelta = {0.25, 0};
    controller.update(camera, input, 0.016f, 1);
    EXPECT_NEAR(camera.position()[0], -8, 0.0001f);
    EXPECT_NEAR(camera.position()[2], 0, 0.0001f);

    input = {};
    input.scrollDelta = 1;
    controller.update(camera, input, 0.016f, 1);
    EXPECT_TRUE(std::abs(camera.position()[0]) < 8);

    input = {};
    input.rightButtonDown = true;
    input.cursorDelta = {0.1, 0};
    controller.update(camera, input, 0.016f, 1);
    EXPECT_TRUE(std::abs(camera.position()[2]) > 0.1f);

    const auto previous = camera.position();
    input.focused = false;
    controller.update(camera, input, 0.016f, 1);
    EXPECT_TRUE(camera.position() == previous);

    Camera orthographic(Camera::Settings{.type = Camera::Type::Orthographic});
    orthographic.setPosition({0, 0, 8});
    OrbitCameraController orthoController;
    orthoController.attach(orthographic);
    input = {};
    input.scrollDelta = 1;
    orthoController.update(orthographic, input, 0.016f, 1);
    EXPECT_TRUE(orthographic.settings().verticalSize < 10);
    EXPECT_NEAR(orthographic.position()[2], 8, 0.0001f);

    Camera fast, slow;
    fast.setPosition({0, 0, 8});
    slow.setPosition({0, 0, 8});
    OrbitCameraController fastController, slowController;
    fastController.attach(fast);
    slowController.attach(slow);
    input = {};
    input.leftButtonDown = true;
    input.cursorDelta = {0.1, 0.05};
    fastController.update(fast, input, 0.01f, 1);
    slowController.update(slow, input, 0.1f, 1);
    EXPECT_TRUE(fast.position() == slow.position());
}

TEST(Viewer, RoutesDragAcrossViewportBoundaries) {
    Scene scene;
    const auto camera = scene.addCamera(Camera{});
    auto window = std::make_unique<TestWindow>();
    auto* state = window.get();
    auto left = std::make_unique<RecordingController>();
    auto right = std::make_unique<RecordingController>();
    auto* leftState = left.get();
    auto* rightState = right.get();
    Viewer viewer({std::move(window), nullptr});
    viewer.addView(scene, camera, {0, 0, 0.5f, 1}, std::move(left));
    viewer.addView(scene, camera, {0.5f, 0, 0.5f, 1}, std::move(right));

    state->input.cursorPosition = {0.75, 0.5};
    state->input.pressPosition = {0.25, 0.5};
    state->input.leftButtonPressed = true;
    state->input.leftButtonDown = true;
    state->input.cursorDelta = {0.05, 0};
    viewer.update(0.016f);
    EXPECT_TRUE(leftState->calls == 1 && rightState->calls == 0);
    EXPECT_NEAR(
        static_cast<float>(leftState->lastInput.cursorDelta[0]), 0.1f, 0.0001f);
    EXPECT_NEAR(leftState->lastAspect, 0.5f, 0.0001f);

    state->input.leftButtonPressed = false;
    viewer.update(0.016f);
    EXPECT_TRUE(leftState->calls == 2 && rightState->calls == 0);

    state->input.pressPosition = {0.75, 0.5};
    state->input.leftButtonPressed = true;
    viewer.update(0.016f);
    EXPECT_TRUE(leftState->calls == 2 && rightState->calls == 1);

    state->input.leftButtonPressed = false;
    state->input.leftButtonDown = false;
    viewer.update(0.016f);
    EXPECT_TRUE(rightState->calls == 2);

    state->input.scrollDelta = 1;
    viewer.update(0.016f);
    EXPECT_TRUE(rightState->calls == 3);
    EXPECT_TRUE(rightState->lastInput.scrollDelta == 1);

    state->input.focused = false;
    viewer.update(0.016f);
    EXPECT_TRUE(rightState->calls == 3);

    state->input.focused = true;
    state->size = {0, 0};
    viewer.update(0.016f);
    EXPECT_TRUE(rightState->calls == 3);
}

TEST(OrbitCameraController, AppliesConfiguredLimits) {
    OrbitCameraControllerSettings settings;
    settings.rotationSpeedRadians = std::numbers::pi_v<float>;
    settings.pitchLimitRadians = 0.2f;

    Camera camera;
    camera.setPosition({0, 0, 8});
    OrbitCameraController controller({}, settings);
    controller.attach(camera);

    InputState input;
    input.leftButtonDown = true;
    input.cursorDelta = {0.5, 1};
    controller.update(camera, input, 0.016f, 1);
    EXPECT_NEAR(camera.position()[0], -8 * std::cos(0.2f), 0.0001f);
    EXPECT_NEAR(camera.position()[1], 8 * std::sin(0.2f), 0.0001f);

    settings.minimumOrthographicSize = 2;
    settings.maximumOrthographicSize = 5;
    Camera orthographic(Camera::Settings{.type = Camera::Type::Orthographic});
    orthographic.setPosition({0, 0, 8});
    OrbitCameraController zoomController({}, settings);
    zoomController.attach(orthographic);

    input = {};
    input.scrollDelta = 100;
    zoomController.update(orthographic, input, 0.016f, 1);
    EXPECT_NEAR(orthographic.settings().verticalSize, 2, 0.0001f);

    input.scrollDelta = -100;
    zoomController.update(orthographic, input, 0.016f, 1);
    EXPECT_NEAR(orthographic.settings().verticalSize, 5, 0.0001f);

    settings.pitchLimitRadians = std::numbers::pi_v<float>;
    EXPECT_THROW((OrbitCameraController({}, settings)), std::invalid_argument);
}
