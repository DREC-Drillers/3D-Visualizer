#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "backendFactory.hpp"
#include "cameraController/orbitCameraController.hpp"
#include "scene/lightning/light.hpp"
#include "visualizer.hpp"
#include "window/glfwWindow.hpp"

namespace {

struct TriangleSettings {
    std::array<float, 3> position;
    float rotationDegrees;
    float tiltDegrees;
    float scale;
    std::array<float, 4> color;
};

visualizer::Quaternion triangleRotation(float rotationDegrees, float tiltDegrees) {
    const float halfRotation = rotationDegrees * std::numbers::pi_v<float> / 360.0f;
    const float halfTilt = tiltDegrees * std::numbers::pi_v<float> / 360.0f;

    const float sinRotation = std::sin(halfRotation);
    const float cosRotation = std::cos(halfRotation);
    const float sinTilt = std::sin(halfTilt);
    const float cosTilt = std::cos(halfTilt);

    // Quaternion product: rotate around Z, then tilt around world Y.
    return {sinTilt * sinRotation,
            sinTilt * cosRotation,
            cosTilt * sinRotation,
            cosTilt * cosRotation};
}

std::array<visualizer::LightId, 3>
configureLighting(visualizer::Lighting& lighting) {
    using namespace visualizer;

    lighting.clear();

    auto ambient = std::make_unique<AmbientLight>();
    ambient->intensity = 0.1f;
    const auto ambientId = lighting.addLight(std::move(ambient));

    auto keyLight = std::make_unique<DirectionalLight>();
    keyLight->towardLight = {-1.0f, 0.7f, 0.8f};
    keyLight->color = {1.0f, 0.55f, 0.2f};
    keyLight->intensity = 0.95f;
    const auto keyId = lighting.addLight(std::move(keyLight));

    auto fillLight = std::make_unique<DirectionalLight>();
    fillLight->towardLight = {1.0f, -0.3f, 0.5f};
    fillLight->color = {0.2f, 0.5f, 1.0f};
    fillLight->intensity = 0.8f;
    const auto fillId = lighting.addLight(std::move(fillLight));

    return {ambientId, keyId, fillId};
}

class LightingDemoControls {
public:
    LightingDemoControls(visualizer::Scene& scene,
                         std::array<visualizer::LightId, 3> lights,
                         GLFWwindow* window)
        : scene_(scene), lights_(lights), window_(window) {
        for (const auto& [id, object] : scene_.objects()) {
            originalColors_.push_back({id, object.material.color});
        }

        updateMaterials();
        showState();

        std::cout << "1: ambient | 2: warm light | 3: cool light\n"
                     "0: all off | 4: all on | L: lighting on/off\n"
                     "C: gray/original colors | Esc: close\n"
                     "Try 0, then 1, 2, 3 individually; compare with L.\n";
    }

    void poll() {
        if (glfwGetWindowAttrib(window_, GLFW_FOCUSED) != GLFW_TRUE) {
            previousKeys_.fill(false);
            return;
        }

        bool changed = false;

        for (std::size_t i = 0; i < KEYS.size(); ++i) {
            const bool down = glfwGetKey(window_, KEYS[i]) == GLFW_PRESS;

            if (down && !previousKeys_[i]) {
                handleKey(KEYS[i]);
                changed = true;
            }

            previousKeys_[i] = down;
        }

        if (changed) {
            updateMaterials();
            showState();
        }
    }

private:
    void handleKey(int key) {
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_3) {
            auto& light = scene_.lighting.light(lights_[key - GLFW_KEY_1]);
            light.enabled = !light.enabled;
        } else if (key == GLFW_KEY_0 || key == GLFW_KEY_4) {
            for (const auto id : lights_) {
                scene_.lighting.light(id).enabled = key == GLFW_KEY_4;
            }
        } else if (key == GLFW_KEY_L) {
            lit_ = !lit_;
        } else if (key == GLFW_KEY_C) {
            gray_ = !gray_;
        } else if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }

    void updateMaterials() {
        for (const auto& [id, color] : originalColors_) {
            auto& material = scene_.object(id).material;
            material.lit = lit_;
            material.color =
                gray_ ? std::array<float, 4>{0.8f, 0.8f, 0.8f, 1.0f} : color;
        }
    }

    void showState() const {
        const auto state = [](bool enabled) { return enabled ? "ON" : "OFF"; };
        constexpr std::array LABELS{"1 Ambient: ", "2 Warm: ", "3 Cool: "};

        std::string title = "Lighting demo";

        for (std::size_t i = 0; i < lights_.size(); ++i) {
            title += " | ";
            title += LABELS[i];
            title += state(scene_.lighting.light(lights_[i]).enabled);
        }

        title += std::string(" | L Shading: ") + state(lit_);
        title += gray_ ? " | C Gray" : " | C Colors";

        glfwSetWindowTitle(window_, title.c_str());
        std::cout << title << '\n';
    }

    visualizer::Scene& scene_;
    std::array<visualizer::LightId, 3> lights_;
    GLFWwindow* window_;

    std::vector<std::pair<visualizer::ObjectId, std::array<float, 4>>>
        originalColors_;

    static constexpr std::array KEYS{GLFW_KEY_1,
                                     GLFW_KEY_2,
                                     GLFW_KEY_3,
                                     GLFW_KEY_0,
                                     GLFW_KEY_4,
                                     GLFW_KEY_L,
                                     GLFW_KEY_C,
                                     GLFW_KEY_ESCAPE};

    std::array<bool, KEYS.size()> previousKeys_{};
    bool lit_ = true;
    bool gray_ = true;
};

void runDemo(visualizer::Viewer& viewer, visualizer::Window& window,
             LightingDemoControls& controls) {
    auto previousTime = std::chrono::steady_clock::now();

    while (!window.shouldClose()) {
        window.pollEvents();
        controls.poll();

        if (window.shouldClose()) {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        const float deltaTime =
            std::chrono::duration<float>(now - previousTime).count();
        previousTime = now;

        viewer.update(std::min(deltaTime, 0.1f));
        viewer.drawFrame();

        const auto size = window.framebufferSize();

        if (size.width <= 0 || size.height <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
}

} // namespace

int main() {
    using namespace visualizer;

    try {
        Scene scene;
        const auto lights = configureLighting(scene.lighting);

        const Geometry geometry{
            .topology = Topology::Triangles,
            .vertices = {{{-0.7f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                         {{0.7f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                         {{0.0f, 0.7f, 0.0f}, {0.0f, 0.0f, 1.0f}}}};

        const std::array<TriangleSettings, 10> triangles{
            {{{-3.6f, 1.3f, -0.8f},
              -25.0f,
              -55.0f,
              0.90f,
              {0.95f, 0.35f, 0.20f, 1.0f}},
             {{-1.8f, 1.1f, 0.3f},
              15.0f,
              -25.0f,
              0.75f,
              {0.95f, 0.65f, 0.15f, 1.0f}},
             {{0.0f, 1.5f, -0.4f}, 45.0f, 0.0f, 1.00f, {0.65f, 0.85f, 0.20f, 1.0f}},
             {{1.8f, 1.2f, 0.7f}, -10.0f, 25.0f, 0.85f, {0.20f, 0.80f, 0.45f, 1.0f}},
             {{3.6f, 1.4f, -0.2f}, 80.0f, 55.0f, 0.95f, {0.15f, 0.75f, 0.80f, 1.0f}},
             {{-3.5f, -1.3f, 0.4f},
              150.0f,
              55.0f,
              0.80f,
              {0.25f, 0.55f, 0.95f, 1.0f}},
             {{-1.7f, -1.1f, -1.0f},
              -45.0f,
              25.0f,
              1.10f,
              {0.45f, 0.35f, 0.95f, 1.0f}},
             {{0.2f, -1.5f, 0.6f}, 30.0f, 0.0f, 0.75f, {0.75f, 0.30f, 0.90f, 1.0f}},
             {{1.9f, -1.2f, -0.5f},
              110.0f,
              -25.0f,
              1.00f,
              {0.95f, 0.35f, 0.65f, 1.0f}},
             {{3.6f, -1.4f, 0.1f},
              -70.0f,
              -55.0f,
              0.85f,
              {0.85f, 0.85f, 0.90f, 1.0f}}}};

        for (const auto& triangleSettings : triangles) {
            SceneObject triangle;
            triangle.geometry = geometry;
            triangle.transform.position = triangleSettings.position;
            triangle.transform.rotation = triangleRotation(
                triangleSettings.rotationDegrees, triangleSettings.tiltDegrees);
            triangle.transform.scale = {triangleSettings.scale,
                                        triangleSettings.scale,
                                        triangleSettings.scale};
            triangle.material.color = triangleSettings.color;

            scene.addObject(std::move(triangle));
        }

        Camera camera;
        camera.setPosition({0.0f, 0.0f, 8.0f});
        const auto cameraId = scene.addCamera(std::move(camera));

        BackendSettings settings;
        settings.window.title = "Triangle viewer - matte lighting";
        settings.window.width = 1280;
        settings.window.height = 720;

        auto backend = BackendFactory::create(settings);
        auto* window = dynamic_cast<GlfwWindow*>(backend.window.get());

        if (!window) {
            throw std::runtime_error("Lighting demo requires a GLFW window");
        }

        LightingDemoControls controls(scene, lights, window->getWindow());

        Viewer viewer(std::move(backend));
        viewer.addView(
            scene, cameraId, {}, std::make_unique<OrbitCameraController>());
        runDemo(viewer, *window, controls);
    } catch (const std::exception& error) {
        std::cerr << "Failed to run the triangle example: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
