#include "scene/camera.hpp"

#include <limits>
#include <stdexcept>

#include <gtest/gtest.h>

using namespace visualizer;

class InvalidCameraValue : public ::testing::TestWithParam<float> {};

TEST_P(InvalidCameraValue, RejectsInvalidNearPlane) {
    Camera::Settings settings;
    settings.nearPlane = GetParam();

    EXPECT_THROW((Camera(settings)), std::invalid_argument);
}

TEST_P(InvalidCameraValue, RejectsInvalidFieldOfView) {
    Camera::Settings settings;
    settings.verticalFovRadians = GetParam();

    EXPECT_THROW((Camera(settings)), std::invalid_argument);
}

TEST_P(InvalidCameraValue, RejectsInvalidOrthographicSize) {
    Camera::Settings settings;
    settings.type = Camera::Type::Orthographic;
    settings.verticalSize = GetParam();

    EXPECT_THROW((Camera(settings)), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(Boundaries, InvalidCameraValue,
                         ::testing::Values(0.0f, -1.0f,
                                           std::numeric_limits<float>::infinity(),
                                           std::numeric_limits<float>::quiet_NaN()));

TEST(Camera, RejectsUnorderedClippingPlanesWithoutChangingSettings) {
    Camera camera;
    auto settings = camera.settings();
    settings.farPlane = settings.nearPlane;

    EXPECT_THROW(camera.setSettings(settings), std::invalid_argument);
    EXPECT_FLOAT_EQ(camera.settings().farPlane, 100.0f);
}

TEST(Camera, RejectsFieldOfViewAtPi) {
    Camera::Settings settings;
    settings.verticalFovRadians = std::numbers::pi_v<float>;

    EXPECT_THROW((Camera(settings)), std::invalid_argument);
}

TEST(Camera, RejectsUnknownProjection) {
    Camera::Settings settings;
    // Deliberately simulate an unknown value from malformed external input.
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    settings.type = static_cast<Camera::Type>(-1);

    EXPECT_THROW((Camera(settings)), std::invalid_argument);
}

TEST(Camera, InvalidPositionDoesNotOverwritePreviousPosition) {
    Camera camera;
    const std::array<float, 3> previous{1, 2, 3};
    camera.setPosition(previous);

    EXPECT_THROW(camera.setPosition({1, std::numeric_limits<float>::infinity(), 3}),
                 std::invalid_argument);
    EXPECT_EQ(camera.position(), previous);
}

TEST(Camera, NormalizesRotationAndRejectsInvalidQuaternions) {
    Camera camera;
    camera.setRotation({0, 0, 0, 5});
    EXPECT_FLOAT_EQ(camera.rotation().w, 1.0f);

    EXPECT_THROW(camera.setRotation({0, 0, 0, 0}), std::invalid_argument);
    EXPECT_THROW(
        camera.setRotation({0, 0, 0, std::numeric_limits<float>::quiet_NaN()}),
        std::invalid_argument);
    EXPECT_FLOAT_EQ(camera.rotation().w, 1.0f);
}
