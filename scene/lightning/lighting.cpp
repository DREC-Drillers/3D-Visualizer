#include "lighting.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace triangles::visualizer {

LightId Lighting::addLight(std::unique_ptr<Light> light) {
    if (!light) {
        throw std::invalid_argument("Cannot add a null light");
    }

    if (nextLightId_ > std::numeric_limits<std::uint32_t>::max()) {
        throw std::overflow_error("Light IDs are exhausted");
    }

    const LightId id{static_cast<std::uint32_t>(nextLightId_)};
    lights_.emplace(id, std::move(light));
    ++nextLightId_;

    return id;
}

Light& Lighting::light(LightId id) {
    return *lights_.at(id);
}

const Light& Lighting::light(LightId id) const {
    return *lights_.at(id);
}

void Lighting::removeLight(LightId id) {
    if (lights_.erase(id) == 0) {
        throw std::out_of_range("Unknown light ID");
    }
}

void Lighting::clear() noexcept {
    // Preserve ID monotonicity so stale IDs cannot address newly added lights.
    lights_.clear();
}

void Lighting::visit(LightVisitor& visitor) const {
    for (const auto& [id, light] : lights_) {
        if (light->enabled) {
            light->accept(visitor);
        }
    }
}

} // namespace triangles::visualizer
