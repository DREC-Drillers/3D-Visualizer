#pragma once

#include <array>
#include <memory>

#include "../backendSettings.hpp"
#include "window.hpp"

struct GLFWwindow;

namespace triangles::visualizer {

// Construct, use and destroy GLFW windows on the main thread.
class GlfwWindow final : public Window {
public:
    GlfwWindow(const WindowSettings& settings, GraphicsApi graphics);
    ~GlfwWindow() override;

    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    void pollEvents() override;
    InputState takeInput() override;
    bool shouldClose() const override;
    FramebufferSize framebufferSize() const override;

    GLFWwindow* getWindow() const noexcept;

private:
    struct Runtime;
    static std::shared_ptr<Runtime> acquireRuntime();

    void setupInputCallbacks();
    void onCursorPosition(double x, double y);
    void onMouseButton(int button, int action);
    void onFocus(bool focused);
    std::array<double, 2> normalizedCursorPosition() const;

    std::shared_ptr<Runtime> runtime_;
    GLFWwindow* window_ = nullptr;
    InputState input_;
    std::array<double, 2> previousCursorPosition_{};
};

} // namespace triangles::visualizer
