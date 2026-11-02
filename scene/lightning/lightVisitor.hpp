#pragma once

namespace triangles::visualizer {

class AmbientLight;
class DirectionalLight;

class LightVisitor {
public:
    virtual ~LightVisitor() = default;

    virtual void visit(const AmbientLight& light) = 0;
    virtual void visit(const DirectionalLight& light) = 0;
};

} // namespace triangles::visualizer
