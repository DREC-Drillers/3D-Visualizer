#include <backendFactory.hpp>
#include <utility>
#include <visualizer.hpp>

int main(int argc, char**) {
    visualizer::Scene scene;
    const auto camera = scene.addCamera(visualizer::Camera{});

    visualizer::SceneObject triangle;
    triangle.geometry.vertices = {
        {{0, 0, 0}, {0, 0, 1}}, {{1, 0, 0}, {0, 0, 1}}, {{0, 1, 0}, {0, 0, 1}}};
    scene.addObject(std::move(triangle));

    // Keep graphics-backend linkage covered without opening a window in tests.
    if (argc > 1) {
        visualizer::Viewer viewer(
            visualizer::BackendFactory::create(visualizer::BackendSettings{}));
        viewer.addView(scene, camera);
    }

    return scene.objects().size() == 1 ? 0 : 1;
}
