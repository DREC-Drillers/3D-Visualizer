#include "renderer/matrices.hpp"

#include <cmath>
#include <glm/vec4.hpp>
#include <stdexcept>

#include <gtest/gtest.h>

using namespace visualizer;

TEST(Matrices, AppliesScaleThenRotationThenTranslation) {
    Transform transform;
    transform.position = {1, 2, 3};
    transform.scale = {2, 3, 4};
    transform.rotation = {0, 0, std::sqrt(0.5f), std::sqrt(0.5f)};

    const auto point = modelMatrix(transform) * glm::vec4(1, 0, 0, 1);

    EXPECT_NEAR(point.x, 1.0f, 0.0001f);
    EXPECT_NEAR(point.y, 4.0f, 0.0001f);
    EXPECT_NEAR(point.z, 3.0f, 0.0001f);
}

TEST(Matrices, ViewMapsCameraPositionToOrigin) {
    Camera camera;
    camera.setPosition({2, 3, 4});
    camera.setRotation({0, 1, 0, 1});

    const auto point = viewMatrix(camera) * glm::vec4(2, 3, 4, 1);

    EXPECT_NEAR(point.x, 0.0f, 0.0001f);
    EXPECT_NEAR(point.y, 0.0f, 0.0001f);
    EXPECT_NEAR(point.z, 0.0f, 0.0001f);
}

TEST(Matrices, RejectsZeroRotationButAllowsFlatteningScale) {
    Transform transform;
    transform.rotation = {0, 0, 0, 0};
    EXPECT_THROW(modelMatrix(transform), std::invalid_argument);

    transform.rotation = {};
    transform.scale = {1, 1, 0};
    const auto point = modelMatrix(transform) * glm::vec4(1, 2, 3, 1);
    EXPECT_FLOAT_EQ(point.z, 0.0f);
}

class ProjectionTest : public ::testing::TestWithParam<Camera::Type> {};

TEST_P(ProjectionTest, MapsClippingPlanesToOpenGLDepthRange) {
    Camera::Settings settings;
    settings.type = GetParam();
    settings.nearPlane = 1;
    settings.farPlane = 10;
    const auto projection = projectionMatrix(Camera(settings), 2);

    const auto nearPoint = projection * glm::vec4(0, 0, -1, 1);
    const auto farPoint = projection * glm::vec4(0, 0, -10, 1);

    EXPECT_NEAR(nearPoint.z / nearPoint.w, -1.0f, 0.0001f);
    EXPECT_NEAR(farPoint.z / farPoint.w, 1.0f, 0.0001f);
}

TEST_P(ProjectionTest, AccountsForViewportAspectRatio) {
    Camera::Settings settings;
    settings.type = GetParam();
    settings.verticalFovRadians = std::numbers::pi_v<float> / 2;
    settings.verticalSize = 2;
    settings.nearPlane = 1;
    settings.farPlane = 10;
    const auto point =
        projectionMatrix(Camera(settings), 2) * glm::vec4(2, 1, -1, 1);

    EXPECT_NEAR(point.x / point.w, 1.0f, 0.0001f);
    EXPECT_NEAR(point.y / point.w, 1.0f, 0.0001f);
}

INSTANTIATE_TEST_SUITE_P(CameraTypes, ProjectionTest,
                         ::testing::Values(Camera::Type::PERSPECTIVE,
                                           Camera::Type::ORTHOGRAPHIC));
