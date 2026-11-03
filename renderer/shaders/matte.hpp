#pragma once

namespace visualizer::shaders {

inline constexpr char matteVertex[] = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 modelViewProjection;
uniform mat4 model;
uniform mat3 normalMatrix;

out vec3 worldPosition;
out vec3 worldNormal;

void main() {
    gl_Position = modelViewProjection * vec4(position, 1.0);
    worldPosition = (model * vec4(position, 1.0)).xyz;
    worldNormal = normalMatrix * normal;
}
)";

inline constexpr char matteFragment[] = R"(
#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;

uniform vec4 materialColor;
uniform bool materialLit;
uniform vec3 ambientColor;

const int MAX_DIRECTIONAL_LIGHTS = 16;

struct DirectionalLightData {
    vec3 towardLight;
    vec3 radiance;
};

uniform int directionalCount;
uniform DirectionalLightData directionalLights[MAX_DIRECTIONAL_LIGHTS];

out vec4 fragmentColor;

void main() {
    // Evaluate derivatives before branching. Missing normals use the face normal.
    vec3 faceNormal = cross(dFdx(worldPosition), dFdy(worldPosition));

    if (!materialLit) {
        fragmentColor = materialColor;
        return;
    }

    vec3 n = worldNormal;

    if (dot(n, n) > 0.0) {
        n = normalize(n);

        if (!gl_FrontFacing) {
            n = -n;
        }
    } else {
        // Screen-space derivatives already orient the normal toward the viewer.
        n = dot(faceNormal, faceNormal) > 0.0
                ? normalize(faceNormal)
                : vec3(0.0);
    }

    vec3 illumination = ambientColor;

    for (int i = 0; i < directionalCount; ++i) {
        vec3 towardLight = directionalLights[i].towardLight;

        if (dot(towardLight, towardLight) > 0.0) {
            float diffuse = max(dot(n, normalize(towardLight)), 0.0);
            illumination += directionalLights[i].radiance * diffuse;
        }
    }

    fragmentColor = vec4(materialColor.rgb * illumination, materialColor.a);
}
)";

} // namespace visualizer::shaders
