#include "scene/scene.hpp"

#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

using namespace visualizer;

TEST(Scene, ObjectUpdatesAreVisibleThroughConstAccess) {
    Scene scene;
    const auto id = scene.addObject(SceneObject{});
    scene.object(id).transform.position = {1, 2, 3};

    EXPECT_EQ(std::as_const(scene).object(id).transform.position,
              (std::array<float, 3>{1, 2, 3}));
    EXPECT_EQ(scene.objects().size(), 1);
}

TEST(Scene, RemovingObjectPreservesOtherObjectsAndCameras) {
    Scene scene;
    const auto first = scene.addObject(SceneObject{});
    const auto second = scene.addObject(SceneObject{});
    const auto camera = scene.addCamera(Camera{});
    scene.removeObject(first);

    EXPECT_THROW(scene.object(first), std::out_of_range);
    EXPECT_THROW(scene.removeObject(first), std::out_of_range);
    EXPECT_NO_THROW(scene.object(second));
    EXPECT_NO_THROW(scene.camera(camera));
    EXPECT_NE(scene.addObject(SceneObject{}), first);
}

TEST(Scene, RemovedCameraIsNotReplacedByNewCamera) {
    Scene scene;
    const auto first = scene.addCamera(Camera{});
    scene.removeCamera(first);
    const auto second = scene.addCamera(Camera{});

    EXPECT_NE(first, second);
    EXPECT_THROW(std::as_const(scene).camera(first), std::out_of_range);
    EXPECT_THROW(scene.removeCamera(first), std::out_of_range);
    EXPECT_NO_THROW(scene.camera(second));
}
