#include <GLFW/glfw3.h>
#include <array>
#include <glad/gl.h>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "renderer/openGLRenderer.hpp"
#include "renderer/shaderProgram.hpp"
#include "renderer/shaders/flatColor.hpp"
#include "scene/scene.hpp"
#include "window/glfwWindow.hpp"

using namespace visualizer;

namespace {

class OpenGLRendering : public ::testing::Test {
protected:
    static constexpr int SIDE = 96;

    void SetUp() override {
        WindowSettings settings;
        settings.width = SIDE;
        settings.height = SIDE;
        settings.visible = false;
        settings.resizable = false;

        ASSERT_NO_THROW(
            window_ = std::make_unique<GlfwWindow>(settings, GraphicsApi::OpenGL));
        glfwMakeContextCurrent(window_->getWindow());
        ASSERT_NE(gladLoadGL(glfwGetProcAddress), 0);
        contextReady_ = true;
        ASSERT_EQ(glfwGetWindowAttrib(window_->getWindow(), GLFW_VISIBLE),
                  GLFW_FALSE);
        ASSERT_NO_THROW(renderer_ = std::make_unique<OpenGLRenderer>(*window_));

        const auto* driver = glGetString(GL_RENDERER);
        ASSERT_NE(driver, nullptr);
        RecordProperty("GL_RENDERER", reinterpret_cast<const char*>(driver));

        glGenFramebuffers(1, &framebuffer_);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

        glGenTextures(1, &color_);
        glBindTexture(GL_TEXTURE_2D, color_);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA8,
                     SIDE,
                     SIDE,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0);

        glGenRenderbuffers(1, &depth_);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SIDE, SIDE);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_);

        ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_DITHER);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        ASSERT_EQ(glGetError(), GL_NO_ERROR);

        scene_.lighting.clear();
        Camera camera(Camera::Settings{.type = Camera::Type::Orthographic,
                                       .verticalSize = 2.0f,
                                       .nearPlane = 0.1f,
                                       .farPlane = 10.0f});
        camera.setPosition({0, 0, 3});
        camera_ = scene_.addCamera(camera);

        SceneObject triangle;
        triangle.geometry.vertices = {{{-0.9f, -0.9f, 0}, {0, 0, 1}},
                                      {{0.9f, -0.9f, 0}, {0, 0, 1}},
                                      {{0, 0.9f, 0}, {0, 0, 1}}};
        triangle.material.color = {1, 1, 1, 1};
        object_ = scene_.addObject(std::move(triangle));
    }

    void TearDown() override {
        if (contextReady_) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer_);
            glDeleteTextures(1, &color_);
            glDeleteRenderbuffers(1, &depth_);
            renderer_.reset();
            EXPECT_EQ(glGetError(), GL_NO_ERROR);
        }

        window_.reset();
    }

    LightId ambient(std::array<float, 3> color, float intensity = 1.0f) {
        auto light = std::make_unique<AmbientLight>();
        light->color = color;
        light->intensity = intensity;
        return scene_.lighting.addLight(std::move(light));
    }

    LightId directional(std::array<float, 3> towardLight,
                        std::array<float, 3> color = {1, 1, 1},
                        float intensity = 1.0f) {
        auto light = std::make_unique<DirectionalLight>();
        light->towardLight = towardLight;
        light->color = color;
        light->intensity = intensity;
        return scene_.lighting.addLight(std::move(light));
    }

    void drawViews(std::span<const RenderView> views) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        ASSERT_NO_THROW(renderer_->render(Frame{views, {SIDE, SIDE}}));
        EXPECT_EQ(glGetError(), GL_NO_ERROR);
    }

    void draw() {
        const std::array views{RenderView{scene_, camera_, {}}};
        drawViews(views);
    }

    void expectPixel(std::array<int, 4> expected, int x = SIDE / 2,
                     int y = SIDE / 2) {
        std::array<unsigned char, 4> pixel{};
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        ASSERT_EQ(glGetError(), GL_NO_ERROR);

        for (std::size_t channel = 0; channel < pixel.size(); ++channel) {
            SCOPED_TRACE("channel " + std::to_string(channel));
            EXPECT_NEAR(static_cast<int>(pixel[channel]), expected[channel], 3);
        }
    }

    std::unique_ptr<GlfwWindow> window_;
    std::unique_ptr<OpenGLRenderer> renderer_;
    Scene scene_;
    CameraId camera_{};
    ObjectId object_{};
    GLuint framebuffer_ = 0;
    GLuint color_ = 0;
    GLuint depth_ = 0;
    bool contextReady_ = false;
};

TEST_F(OpenGLRendering, LitSurfaceIsBlackWithoutSources) {
    draw();
    expectPixel({0, 0, 0, 255});
    expectPixel({20, 26, 36, 255}, 2, 2);
}

TEST_F(OpenGLRendering, AppliesAmbientColorAndIntensity) {
    ambient({0.2f, 0.4f, 0.6f}, 0.5f);
    draw();
    expectPixel({26, 51, 77, 255});
}

TEST_F(OpenGLRendering, NormalizesDirectionalLightAndAppliesLambertCosine) {
    const auto light = directional({0, 0, 7}, {1, 1, 1}, 0.6f);
    draw();
    expectPixel({153, 153, 153, 255});

    auto& sun = static_cast<DirectionalLight&>(scene_.lighting.light(light));
    sun.towardLight = {1, 0, 0};
    draw();
    expectPixel({0, 0, 0, 255});

    sun.towardLight = {0, 0, -1};
    draw();
    expectPixel({0, 0, 0, 255});
}

TEST_F(OpenGLRendering, AddsColoredSourcesAndUpdatesCountsBetweenFrames) {
    ambient({0, 0.1f, 0});
    const auto red = directional({0, 0, 1}, {1, 0, 0}, 0.25f);
    const auto blue = directional({0, 0, 1}, {0, 0, 1}, 0.5f);
    draw();
    expectPixel({64, 26, 128, 255});

    scene_.lighting.light(red).enabled = false;
    draw();
    expectPixel({0, 26, 128, 255});

    scene_.lighting.removeLight(blue);
    draw();
    expectPixel({0, 26, 0, 255});

    scene_.lighting.clear();
    draw();
    expectPixel({0, 0, 0, 255});
}

TEST_F(OpenGLRendering, SupportsMaximumDirectionalSourceCount) {
    for (std::size_t i = 0; i < maxOpenGLDirectionalLights; ++i) {
        directional({0, 0, 1}, {1, 1, 1}, 0.03125f);
    }

    draw();
    expectPixel({128, 128, 128, 255});
}

TEST_F(OpenGLRendering, ZeroLightDirectionContributesNothing) {
    directional({0, 0, 0});
    draw();
    expectPixel({0, 0, 0, 255});
}

TEST_F(OpenGLRendering, UnlitMaterialPreservesColorAndAlpha) {
    auto& material = scene_.object(object_).material;
    material.lit = false;
    material.color = {0.25f, 0.5f, 0.75f, 0.5f};
    draw();
    expectPixel({64, 128, 191, 128});
}

TEST_F(OpenGLRendering, BackFaceFlipsNormalAndRemainsVisible) {
    auto& vertices = scene_.object(object_).geometry.vertices;
    std::swap(vertices[1], vertices[2]);
    directional({0, 0, -1}, {1, 1, 1}, 0.5f);
    glEnable(GL_CULL_FACE);
    draw();
    expectPixel({128, 128, 128, 255});
}

TEST_F(OpenGLRendering, MissingNormalsUseGeometricNormalOnBothSides) {
    auto& vertices = scene_.object(object_).geometry.vertices;
    for (auto& vertex : vertices) {
        vertex.normal = {};
    }

    directional({0, 0, 1}, {1, 1, 1}, 0.5f);
    draw();
    expectPixel({128, 128, 128, 255});

    std::swap(vertices[1], vertices[2]);
    draw();
    expectPixel({128, 128, 128, 255});
}

TEST_F(OpenGLRendering, RotationChangesDiffuseBrightness) {
    scene_.object(object_).transform.rotation = {0, 0.5f, 0, 0.8660254f};
    directional({0, 0, 1}, {1, 1, 1}, 0.6f);
    draw();
    expectPixel({77, 77, 77, 255});
}

TEST_F(OpenGLRendering, NonuniformScaleUsesInverseTransposeForNormals) {
    auto& triangle = scene_.object(object_);
    triangle.geometry.vertices = {{{-0.7f, -0.7f, 0.7f}, {1, 0, 1}},
                                  {{0.7f, -0.7f, -0.7f}, {1, 0, 1}},
                                  {{0, 0.7f, 0}, {1, 0, 1}}};
    triangle.transform.scale = {2, 1, 0.5f};
    directional({0, 0, 1}, {1, 1, 1}, 0.75f);
    draw();
    expectPixel({186, 186, 186, 255});
}

TEST_F(OpenGLRendering, SingularScaleUsesGeometricNormalFallback) {
    scene_.object(object_).transform.scale = {1, 1, 0};
    directional({0, 0, 1}, {1, 1, 1}, 0.5f);
    draw();
    expectPixel({128, 128, 128, 255});
}

TEST_F(OpenGLRendering, DepthKeepsNearTriangleVisibleWhenFarOneIsDrawnLast) {
    auto& near = scene_.object(object_);
    near.material.lit = false;
    near.material.color = {1, 0, 0, 1};
    near.transform.position[2] = 0.5f;
    auto far = near;
    far.material.color = {0, 0, 1, 1};
    far.transform.position[2] = -0.5f;
    scene_.addObject(std::move(far));
    draw();
    expectPixel({255, 0, 0, 255});
}

TEST_F(OpenGLRendering, ViewportsUseTopOriginAndIndependentSceneLighting) {
    ambient({0.4f, 0, 0});
    Scene other;
    other.lighting.clear();
    auto blue = std::make_unique<AmbientLight>();
    blue->color = {0, 0, 0.6f};
    other.lighting.addLight(std::move(blue));
    const auto camera = other.addCamera(scene_.camera(camera_));
    other.addObject(scene_.object(object_));
    const std::array views{RenderView{scene_, camera_, {0, 0, 1, 0.5f}},
                           RenderView{other, camera, {0, 0.5f, 1, 0.5f}}};
    drawViews(views);
    expectPixel({102, 0, 0, 255}, SIDE / 2, 3 * SIDE / 4);
    expectPixel({0, 0, 153, 255}, SIDE / 2, SIDE / 4);

    std::array<GLint, 4> scissor{};
    glGetIntegerv(GL_SCISSOR_BOX, scissor.data());
    EXPECT_EQ(scissor, (std::array<GLint, 4>{0, 0, SIDE, SIDE / 2}));

    glEnable(GL_SCISSOR_TEST);
    drawViews({});
    expectPixel({20, 26, 36, 255}, SIDE / 2, 3 * SIDE / 4);
}

TEST_F(OpenGLRendering, ObjectTransformMovesGeometryOutsideViewport) {
    scene_.object(object_).material.lit = false;
    draw();
    expectPixel({255, 255, 255, 255});

    scene_.object(object_).transform.position[0] = 5;
    draw();
    expectPixel({20, 26, 36, 255});
}

TEST_F(OpenGLRendering, EmptyGeometryAndZeroSizeFrameAreHandled) {
    scene_.object(object_).material.lit = false;
    draw();
    const std::array views{RenderView{scene_, camera_, {}}};
    renderer_->render(Frame{views, {0, 0}});
    expectPixel({255, 255, 255, 255});

    scene_.object(object_).geometry.vertices.clear();
    draw();
    expectPixel({20, 26, 36, 255});
}

TEST_F(OpenGLRendering, ShaderFailuresReportExceptionsWithoutBreakingNextDraw) {
    constexpr char BAD_SHADER[] = "#version 330 core\nvoid main() { invalid_glsl; }";
    EXPECT_THROW((ShaderProgram(BAD_SHADER, shaders::flatColorFragment)),
                 std::runtime_error);
    EXPECT_THROW((ShaderProgram(shaders::flatColorVertex, BAD_SHADER)),
                 std::runtime_error);

    ShaderProgram flat(shaders::flatColorVertex, shaders::flatColorFragment);
    EXPECT_GE(flat.uniformLocation("materialColor"), 0);
    EXPECT_EQ(flat.uniformLocation("missingUniform"), -1);

    scene_.object(object_).material.lit = false;
    draw();
    expectPixel({255, 255, 255, 255});
}

TEST_F(OpenGLRendering, RendererResourcesCanBeRecreatedInSameContext) {
    scene_.object(object_).material.lit = false;
    draw();
    renderer_.reset();
    ASSERT_NO_THROW(renderer_ = std::make_unique<OpenGLRenderer>(*window_));
    draw();
    expectPixel({255, 255, 255, 255});
}

class UnlitPrimitive : public OpenGLRendering,
                       public ::testing::WithParamInterface<Topology> {};

TEST_P(UnlitPrimitive, KeepsMaterialColorWithoutLightsOrNormals) {
    // Place the primitive through a pixel center, away from rasterization ties.
    constexpr float PIXEL_CENTER = 1.0f / SIDE;
    auto& geometry = scene_.object(object_).geometry;
    geometry.topology = GetParam();
    geometry.vertices = {{{PIXEL_CENTER, PIXEL_CENTER, 0}, {}}};
    if (GetParam() == Topology::Lines) {
        geometry.vertices = {{{-0.8f, PIXEL_CENTER, 0}, {}},
                             {{0.8f, PIXEL_CENTER, 0}, {}}};
    }

    draw();
    expectPixel({255, 255, 255, 255});
}

INSTANTIATE_TEST_SUITE_P(Topologies, UnlitPrimitive,
                         ::testing::Values(Topology::Points, Topology::Lines));

} // namespace
