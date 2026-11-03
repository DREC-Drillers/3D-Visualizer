#include <memory>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include "renderer/openGLLightDataCollector.hpp"
#include "scene/scene.hpp"

using namespace visualizer;

namespace {

OpenGLLightingData collect(const Lighting& lighting) {
    OpenGLLightingData data;
    OpenGLLightDataCollector collector(data);
    lighting.visit(collector);

    return data;
}

} // namespace

TEST(Lighting, EmptyCollectionContributesNoLight) {
    const Lighting lighting;
    const auto data = collect(lighting);

    EXPECT_EQ(data.ambientColor, (std::array<float, 3>{}));
    EXPECT_EQ(data.directionalCount, 0);
}

TEST(Lighting, RejectsNullSource) {
    Lighting lighting;

    EXPECT_THROW(lighting.addLight(nullptr), std::invalid_argument);
}

TEST(Lighting, SumsAmbientColorsAndIntensities) {
    Lighting lighting;
    auto ambient = std::make_unique<AmbientLight>();
    ambient->color = {1.0f, 0.5f, 0.0f};
    ambient->intensity = 0.2f;
    lighting.addLight(std::move(ambient));
    lighting.addLight(std::make_unique<AmbientLight>());

    const auto data = collect(lighting);

    EXPECT_FLOAT_EQ(data.ambientColor[0], 1.2f);
    EXPECT_FLOAT_EQ(data.ambientColor[1], 1.1f);
    EXPECT_FLOAT_EQ(data.ambientColor[2], 1.0f);
}

TEST(Lighting, CollectsDistinctDirectionalSources) {
    Lighting lighting;
    auto sun = std::make_unique<DirectionalLight>();
    sun->towardLight = {1, 2, 3};
    sun->color = {0.2f, 0.4f, 0.6f};
    sun->intensity = 0.5f;
    lighting.addLight(std::move(sun));
    lighting.addLight(std::make_unique<DirectionalLight>());

    const auto data = collect(lighting);
    ASSERT_EQ(data.directionalCount, 2);

    EXPECT_EQ(data.directional[0].towardLight, (std::array<float, 3>{1, 2, 3}));
    EXPECT_FLOAT_EQ(data.directional[0].radiance[2], 0.3f);
    EXPECT_FLOAT_EQ(data.directional[1].radiance[2], 1.0f);
}

TEST(Lighting, SkipsDisabledSourcesOfBothTypes) {
    Lighting lighting;
    const auto ambient = lighting.addLight(std::make_unique<AmbientLight>());
    const auto sun = lighting.addLight(std::make_unique<DirectionalLight>());
    lighting.light(ambient).enabled = false;
    lighting.light(sun).enabled = false;

    const auto data = collect(lighting);

    EXPECT_EQ(data.ambientColor, (std::array<float, 3>{}));
    EXPECT_EQ(data.directionalCount, 0);
}

TEST(Lighting, RemovedIdsAreInvalidAndNotReused) {
    Lighting lighting;
    const auto old = lighting.addLight(std::make_unique<AmbientLight>());
    lighting.removeLight(old);
    const auto next = lighting.addLight(std::make_unique<AmbientLight>());

    EXPECT_NE(old, next);
    EXPECT_THROW(lighting.light(old), std::out_of_range);
    EXPECT_THROW(std::as_const(lighting).light(old), std::out_of_range);
    EXPECT_THROW(lighting.removeLight(old), std::out_of_range);
}

TEST(Lighting, ClearInvalidatesSourcesWithoutReusingIds) {
    Lighting lighting;
    const auto old = lighting.addLight(std::make_unique<AmbientLight>());
    lighting.clear();

    EXPECT_EQ(collect(lighting).ambientColor, (std::array<float, 3>{}));
    EXPECT_THROW(lighting.light(old), std::out_of_range);
    EXPECT_NE(lighting.addLight(std::make_unique<AmbientLight>()), old);
}

TEST(Lighting, MovePreservesOwnedSources) {
    Lighting lighting;
    const auto id = lighting.addLight(std::make_unique<DirectionalLight>());
    auto moved = std::move(lighting);

    EXPECT_TRUE(std::as_const(moved).light(id).enabled);
    EXPECT_EQ(collect(moved).directionalCount, 1);
}

TEST(OpenGLLightDataCollector, EnforcesEnabledDirectionalSourceLimit) {
    Lighting lighting;
    for (std::size_t i = 0; i < maxOpenGLDirectionalLights; ++i) {
        lighting.addLight(std::make_unique<DirectionalLight>());
    }

    EXPECT_EQ(collect(lighting).directionalCount, maxOpenGLDirectionalLights);

    const auto extra = lighting.addLight(std::make_unique<DirectionalLight>());
    EXPECT_THROW(collect(lighting), std::length_error);

    lighting.light(extra).enabled = false;
    EXPECT_EQ(collect(lighting).directionalCount, maxOpenGLDirectionalLights);
}

TEST(Scene, DefaultLightingContainsAmbientAndDirectionalSources) {
    const Scene scene;
    const auto data = collect(scene.lighting);

    EXPECT_FLOAT_EQ(data.ambientColor[0], 0.25f);
    ASSERT_EQ(data.directionalCount, 1);
    EXPECT_FLOAT_EQ(data.directional[0].radiance[0], 0.75f);
}
