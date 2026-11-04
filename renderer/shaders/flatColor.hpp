#pragma once

namespace visualizer::shaders {

// Static, null-terminated GLSL text passed to the OpenGL C API.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
inline constexpr char FLAT_COLOR_VERTEX[] = R"(
#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 modelViewProjection;
void main() { gl_Position = modelViewProjection * vec4(position, 1.0); }
)";

// Static, null-terminated GLSL text passed to the OpenGL C API.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
inline constexpr char FLAT_COLOR_FRAGMENT[] = R"(
#version 330 core
uniform vec4 materialColor;
out vec4 fragmentColor;
void main() { fragmentColor = materialColor; }
)";

} // namespace visualizer::shaders
