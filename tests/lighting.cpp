#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

#include "renderer/openGLLightDataCollector.hpp"
#include "scene/scene.hpp"

using namespace visualizer;

namespace {

void require(bool condition) {
    if (!condition) {
        throw std::runtime_error("Lighting verification failed");
    }
}

bool close(float a, float b) {
    return std::abs(a - b) < 0.0001f;
}

template <class Exception, class Function>
void requireThrows(Function function) {
    try {
        function();
    } catch (const Exception&) {
        return;
    }

    throw std::runtime_error("Expected lighting exception");
}

OpenGLLightingData collect(const Lighting& lighting) {
    OpenGLLightingData data;
    OpenGLLightDataCollector collector(data);
    lighting.visit(collector);

    return data;
}

} // namespace

int main() {
    Lighting lighting;
    require(collect(lighting).directionalCount == 0);
    require(collect(lighting).ambientColor == std::array<float, 3>{});
    requireThrows<std::invalid_argument>([&] { lighting.addLight(nullptr); });

    auto ambient = std::make_unique<AmbientLight>();
    ambient->color = {1.0f, 0.5f, 0.0f};
    ambient->intensity = 0.2f;
    const auto ambientId = lighting.addLight(std::move(ambient));
    lighting.addLight(std::make_unique<AmbientLight>());

    auto sun = std::make_unique<DirectionalLight>();
    sun->towardLight = {1.0f, 2.0f, 3.0f};
    sun->color = {0.2f, 0.4f, 0.6f};
    sun->intensity = 0.5f;
    const auto sunId = lighting.addLight(std::move(sun));

    const auto data = collect(lighting);
    require(close(data.ambientColor[0], 1.2f));
    require(close(data.ambientColor[1], 1.1f));
    require(close(data.ambientColor[2], 1.0f));

    require(data.directionalCount == 1);
    require(data.directional[0].towardLight == std::array<float, 3>{1, 2, 3});
    require(close(data.directional[0].radiance[2], 0.3f));

    lighting.light(sunId).enabled = false;
    lighting.light(ambientId).enabled = false;

    require(collect(lighting).directionalCount == 0);
    require(close(collect(lighting).ambientColor[0], 1.0f));

    lighting.removeLight(sunId);
    requireThrows<std::out_of_range>([&] { lighting.removeLight(sunId); });
    requireThrows<std::out_of_range>([&] { lighting.light(sunId); });

    lighting.clear();
    require(collect(lighting).ambientColor == std::array<float, 3>{});

    const auto newId = lighting.addLight(std::make_unique<AmbientLight>());
    require(newId != ambientId && newId != sunId);

    const Lighting& constLighting = lighting;
    require(constLighting.light(newId).enabled);

    auto moved = std::move(lighting);
    require(moved.light(newId).enabled);

    Lighting many;
    for (std::size_t i = 0; i < maxOpenGLDirectionalLights; ++i) {
        many.addLight(std::make_unique<DirectionalLight>());
    }

    require(collect(many).directionalCount == maxOpenGLDirectionalLights);

    const auto extra = many.addLight(std::make_unique<DirectionalLight>());
    requireThrows<std::length_error>([&] { collect(many); });

    many.light(extra).enabled = false;
    require(collect(many).directionalCount == maxOpenGLDirectionalLights);

    Scene scene;
    const auto defaults = collect(scene.lighting);
    require(close(defaults.ambientColor[0], 0.25f));
    require(defaults.directionalCount == 1);
    require(close(defaults.directional[0].radiance[0], 0.75f));
}
