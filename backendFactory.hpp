#pragma once

#include "backendSettings.hpp"
#include "visualizerBackend.hpp"

namespace visualizer {

class BackendFactory {
public:
    static ViewerBackend create(const BackendSettings& settings);
};

} // namespace visualizer
