#pragma once

#include <array>

namespace visualizer {

struct InputState {
    std::array<double, 2> cursorPosition{};
    std::array<double, 2> pressPosition{};
    std::array<double, 2> cursorDelta{};
    double scrollDelta = 0.0;

    bool leftButtonDown = false;
    bool rightButtonDown = false;
    bool leftButtonPressed = false;
    bool rightButtonPressed = false;
    bool focused = true;
};

} // namespace visualizer
