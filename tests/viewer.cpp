#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "cameraController/orbitCameraController.hpp"
#include "core/visualizer.hpp"

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

class LifetimeWindow final : public Window {
public:
    explicit LifetimeWindow(bool& alive) : alive_(alive) { alive_ = true; }
    ~LifetimeWindow() override { alive_ = false; }

    void pollEvents() override {}
    InputState takeInput() override { return {}; }
    bool shouldClose() const override { return true; }
    FramebufferSize framebufferSize() const override { return {800, 600}; }

private:
    bool& alive_;
};

class LifetimeRenderer final : public Renderer {
public:
    LifetimeRenderer(const bool& windowAlive, int& destructions)
        : windowAlive_(windowAlive), destructions_(destructions) {}

    ~LifetimeRenderer() override {
        EXPECT_TRUE(windowAlive_) << "Renderer must be destroyed before its window";
        ++destructions_;
    }

    void render(const Frame&) override { EXPECT_TRUE(windowAlive_); }

private:
    const bool& windowAlive_;
    int& destructions_;
};

ViewerBackend lifetimeBackend(bool& windowAlive, int& destructions) {
    auto window = std::make_unique<LifetimeWindow>(windowAlive);
    auto renderer = std::make_unique<LifetimeRenderer>(windowAlive, destructions);

    return {std::move(window), std::move(renderer)};
}

class AttachCountingController final : public CameraController {
public:
    explicit AttachCountingController(int& calls) : calls_(calls) {}
    void attach(const Camera&) override { ++calls_; }
    void update(Camera&, const InputState&, float, float) override {}

private:
    int& calls_;
};

} // namespace

TEST(Viewer, ReportsMissingBackendWithoutEnteringRunLoop) {
    Viewer viewer(ViewerBackend{});

    EXPECT_THROW(viewer.update(0.01f), std::logic_error);
    EXPECT_THROW(viewer.drawFrame(), std::logic_error);
    EXPECT_THROW(viewer.run(), std::logic_error);
}

class InvalidViewportTest : public ::testing::TestWithParam<NormalizedViewport> {};

TEST_P(InvalidViewportTest, RejectsInvalidViewport) {
    Scene scene;
    const auto camera = scene.addCamera(Camera{});
    Viewer viewer(ViewerBackend{});

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
    Viewer viewer(ViewerBackend{});
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

static_assert(!std::is_copy_constructible_v<ViewerBackend>);
static_assert(!std::is_copy_assignable_v<ViewerBackend>);
static_assert(std::is_nothrow_move_constructible_v<ViewerBackend>);
static_assert(std::is_nothrow_move_assignable_v<ViewerBackend>);

TEST(ViewerBackend, MoveAssignmentReleasesRendererBeforeWindow) {
    bool oldAlive = false;
    bool newAlive = false;
    int destructions = 0;

    {
        auto target = lifetimeBackend(oldAlive, destructions);
        auto source = lifetimeBackend(newAlive, destructions);
        auto* newWindow = source.window.get();
        auto* newRenderer = source.renderer.get();

        auto& assigned = (target = std::move(source));

        EXPECT_EQ(&assigned, &target);
        EXPECT_FALSE(oldAlive);
        EXPECT_TRUE(newAlive);
        EXPECT_EQ(destructions, 1);
        EXPECT_EQ(target.window.get(), newWindow);
        EXPECT_EQ(target.renderer.get(), newRenderer);
        // Verify the documented empty state of the moved unique_ptr members.
        // NOLINTBEGIN(bugprone-use-after-move)
        EXPECT_EQ(source.window, nullptr);
        EXPECT_EQ(source.renderer, nullptr);
        // NOLINTEND(bugprone-use-after-move)
    }

    EXPECT_FALSE(newAlive);
    EXPECT_EQ(destructions, 2);
}

TEST(ViewerBackend, MoveConstructionPreservesResources) {
    bool alive = false;
    int destructions = 0;

    {
        auto source = lifetimeBackend(alive, destructions);
        ViewerBackend target(std::move(source));

        EXPECT_TRUE(alive);
        EXPECT_EQ(destructions, 0);
        // Verify the documented empty state of the moved unique_ptr members.
        // NOLINTBEGIN(bugprone-use-after-move)
        EXPECT_EQ(source.window, nullptr);
        EXPECT_EQ(source.renderer, nullptr);
        // NOLINTEND(bugprone-use-after-move)
        target.renderer->render(Frame{});
    }

    EXPECT_FALSE(alive);
    EXPECT_EQ(destructions, 1);
}

static_assert(!std::is_default_constructible_v<Viewer>);
static_assert(!std::is_copy_constructible_v<Viewer>);
static_assert(!std::is_copy_assignable_v<Viewer>);
static_assert(std::is_nothrow_move_constructible_v<Viewer>);
static_assert(!std::is_move_assignable_v<Viewer>);

TEST(Viewer, MoveConstructionPreservesBackendLifetime) {
    bool alive = false;
    int destructions = 0;

    {
        Viewer source(lifetimeBackend(alive, destructions));
        Viewer target(std::move(source));

        EXPECT_TRUE(alive);
        EXPECT_EQ(destructions, 0);
        EXPECT_NO_THROW(target.drawFrame());
    }

    EXPECT_FALSE(alive);
    EXPECT_EQ(destructions, 1);
}

TEST(Viewer, RejectsSecondControllerBeforeAttachingIt) {
    Scene scene;
    const auto camera = scene.addCamera(Camera{});
    Viewer viewer(ViewerBackend{});
    int attachments = 0;

    viewer.addView(scene,
                   camera,
                   {0, 0, 0.5f, 1},
                   std::make_unique<AttachCountingController>(attachments));
    EXPECT_EQ(attachments, 1);

    EXPECT_THROW(
        viewer.addView(scene,
                       camera,
                       {0.5f, 0, 0.5f, 1},
                       std::make_unique<AttachCountingController>(attachments)),
        std::invalid_argument);
    EXPECT_EQ(attachments, 1);

    // The rejected addition must leave this viewport available.
    EXPECT_NO_THROW(viewer.addView(scene, camera, {0.5f, 0, 0.5f, 1}));
}

TEST(Viewer, AllowsOneControllerAlongsideUncontrolledViewsOfSameCamera) {
    Scene scene;
    const auto camera = scene.addCamera(Camera{});
    Viewer viewer(ViewerBackend{});
    int attachments = 0;

    viewer.addView(scene, camera, {0, 0, 0.25f, 1});
    EXPECT_NO_THROW(
        viewer.addView(scene,
                       camera,
                       {0.25f, 0, 0.5f, 1},
                       std::make_unique<AttachCountingController>(attachments)));
    EXPECT_NO_THROW(viewer.addView(scene, camera, {0.75f, 0, 0.25f, 1}));
    EXPECT_EQ(attachments, 1);
}

TEST(Viewer, AllowsControllersForDistinctCamerasAndScenes) {
    Scene first;
    Scene second;
    const auto camera = first.addCamera(Camera{});
    const auto otherCamera = first.addCamera(Camera{});
    const auto sameIdInOtherScene = second.addCamera(Camera{});
    ASSERT_EQ(camera, sameIdInOtherScene);
    Viewer viewer(ViewerBackend{});
    int attachments = 0;

    viewer.addView(first,
                   camera,
                   {0, 0, 0.25f, 1},
                   std::make_unique<AttachCountingController>(attachments));
    EXPECT_NO_THROW(
        viewer.addView(first,
                       otherCamera,
                       {0.25f, 0, 0.25f, 1},
                       std::make_unique<AttachCountingController>(attachments)));
    EXPECT_NO_THROW(
        viewer.addView(second,
                       sameIdInOtherScene,
                       {0.5f, 0, 0.5f, 1},
                       std::make_unique<AttachCountingController>(attachments)));
    EXPECT_EQ(attachments, 3);
}
