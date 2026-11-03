#include "glfwWindow.hpp"

#include <limits>
#include <stdexcept>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace visualizer {

struct GlfwWindow::Runtime {
    Runtime() {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
    }

    ~Runtime() { glfwTerminate(); }
};

std::shared_ptr<GlfwWindow::Runtime> GlfwWindow::acquireRuntime() {
    static std::weak_ptr<Runtime> current;
    auto runtime = current.lock();

    if (!runtime) {
        runtime = std::make_shared<Runtime>();
        current = runtime;
    }

    return runtime;
}

GlfwWindow::GlfwWindow(const WindowSettings& settings, GraphicsApi graphics) {
    const auto maxSize = static_cast<std::uint32_t>(std::numeric_limits<int>::max());
    if (settings.width == 0 || settings.height == 0 || settings.width > maxSize ||
        settings.height > maxSize) {
        throw std::invalid_argument(
            "Window dimensions must be positive and fit in int");
    }

    runtime_ = acquireRuntime();

    glfwDefaultWindowHints();
    switch (graphics) {
        case GraphicsApi::OpenGL:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
            break;
        case GraphicsApi::Vulkan:
            throw std::invalid_argument("not implemented yet");
            break;
        default:
            throw std::invalid_argument("Unknown graphics API");
    }

    glfwWindowHint(GLFW_RESIZABLE, settings.resizable ? GLFW_TRUE : GLFW_FALSE);

    window_ = glfwCreateWindow(static_cast<int>(settings.width),
                               static_cast<int>(settings.height),
                               settings.title.c_str(),
                               nullptr,
                               nullptr);
    if (!window_) {
        throw std::runtime_error(
            "Failed to create GLFW window for the selected graphics API");
    }

    setupInputCallbacks();
}

GlfwWindow::~GlfwWindow() {
    glfwDestroyWindow(window_);
}

void GlfwWindow::pollEvents() {
    glfwPollEvents();
}

InputState GlfwWindow::takeInput() {
    input_.cursorPosition = normalizedCursorPosition();
    const auto frameInput = input_;

    input_.cursorDelta = {};
    input_.scrollDelta = 0;
    input_.leftButtonPressed = false;
    input_.rightButtonPressed = false;

    return frameInput;
}

std::array<double, 2> GlfwWindow::normalizedCursorPosition() const {
    double x, y;
    int width, height;
    glfwGetCursorPos(window_, &x, &y);
    glfwGetWindowSize(window_, &width, &height);

    if (width <= 0 || height <= 0) {
        return {-1, -1};
    }

    return {x / width, y / height};
}

void GlfwWindow::setupInputCallbacks() {
    glfwSetWindowUserPointer(window_, this);
    glfwGetCursorPos(
        window_, &previousCursorPosition_[0], &previousCursorPosition_[1]);
    input_.focused = glfwGetWindowAttrib(window_, GLFW_FOCUSED) == GLFW_TRUE;

    glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double x, double y) {
        auto& self = *static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        self.onCursorPosition(x, y);
    });

    glfwSetMouseButtonCallback(
        window_, [](GLFWwindow* window, int button, int action, int) {
            auto& self = *static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
            self.onMouseButton(button, action);
        });

    glfwSetScrollCallback(window_, [](GLFWwindow* window, double, double y) {
        auto& self = *static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        if (self.input_.focused) {
            self.input_.scrollDelta += y;
        }
    });

    glfwSetWindowFocusCallback(window_, [](GLFWwindow* window, int focused) {
        auto& self = *static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
        self.onFocus(focused == GLFW_TRUE);
    });
}

void GlfwWindow::onCursorPosition(double x, double y) {
    int width, height;
    glfwGetWindowSize(window_, &width, &height);

    if (input_.focused && width > 0 && height > 0) {
        input_.cursorDelta[0] += (x - previousCursorPosition_[0]) / width;
        input_.cursorDelta[1] += (y - previousCursorPosition_[1]) / height;
    }

    previousCursorPosition_ = {x, y};
}

void GlfwWindow::onMouseButton(int button, int action) {
    if (!input_.focused) {
        return;
    }

    if (action == GLFW_PRESS &&
        (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT)) {
        if (!input_.leftButtonDown && !input_.rightButtonDown) {
            input_.pressPosition = normalizedCursorPosition();
            input_.cursorDelta = {};
            glfwGetCursorPos(
                window_, &previousCursorPosition_[0], &previousCursorPosition_[1]);
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        input_.leftButtonDown = action == GLFW_PRESS;
        input_.leftButtonPressed |= action == GLFW_PRESS;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        input_.rightButtonDown = action == GLFW_PRESS;
        input_.rightButtonPressed |= action == GLFW_PRESS;
    }
}

void GlfwWindow::onFocus(bool focused) {
    input_ = {};
    input_.focused = focused;
    glfwGetCursorPos(
        window_, &previousCursorPosition_[0], &previousCursorPosition_[1]);
}

bool GlfwWindow::shouldClose() const {
    return glfwWindowShouldClose(window_) == GLFW_TRUE;
}

FramebufferSize GlfwWindow::framebufferSize() const {
    FramebufferSize size{};
    glfwGetFramebufferSize(window_, &size.width, &size.height);

    return size;
}

GLFWwindow* GlfwWindow::getWindow() const noexcept {
    return window_;
}

} // namespace visualizer
