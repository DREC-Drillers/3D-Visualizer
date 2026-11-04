#pragma once

namespace visualizer::shaders {

inline constexpr char FLAT_COLOR_VERTEX[] = R"(
#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 modelViewProjection;
void main() { gl_Position = modelViewProjection * vec4(position, 1.0); }
)";

inline constexpr char FLAT_COLOR_FRAGMENT[] = R"(
#version 330 core
uniform vec4 materialColor;
out vec4 fragmentColor;
void main() { fragmentColor = materialColor; }
)";

} // namespace visualizer::shaders
