#pragma once

#include <compare>
#include <cstdint>
#include <map>
#include <memory>

#include "light.hpp"

namespace triangles::visualizer {

struct LightId {
    std::uint32_t value;

    auto operator<=>(const LightId&) const = default;
};

// An empty collection contributes no illumination. Owns its lights exclusively.
class Lighting {
public:
    LightId addLight(std::unique_ptr<Light> light);

    Light& light(LightId id);
    const Light& light(LightId id) const;

    void removeLight(LightId id);
    void clear() noexcept;

    void visit(LightVisitor& visitor) const;

private:
    std::map<LightId, std::unique_ptr<Light>> lights_;
    std::uint64_t nextLightId_ = 0;
};

} // namespace triangles::visualizer
