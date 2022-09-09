#include "GL/Shader.hpp"
#include "Layout.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "opengl/core.hpp"
#include "platform/Window.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "platform/windows/glcore.hpp"
#include <memory>
#include <sstream>

namespace {

struct UiTest : plt::WindowStateCallback {
    Shader shader;
    Shader::LocationHandle posAttrib;

    ui::Layout layout;
    ui::LayoutBox box1;
    ui::RectEqn box1Eqn;

    UiTest()
        : box1(layout) {
        box1Eqn = layout.align(box1.outerBox, layout.screen());
    }

    void onInitialize(plt::Window*) override {
        ShaderBuilder builder;
        posAttrib = builder.addAttrib("vec4", "a_position");
        builder.vMain << "gl_Position = a_position;\n";
        builder.fMain << Shader::FRAGMENT_COLOR << " = vec4(1.0, 1.0, 1.0, 1.0);\n";
        shader.initialize(builder);
    }

    void onVisible(plt::Window* window) override {
        using namespace ui::code;
        box1.localValues.padding.y() = 100_px;
        box1.setStyle(ui::LayoutBoxStyle { .padding = { { 20_pt }, {} } });
        layout.doLayout(window->width(), window->height(), 0.5f);
        layout.debug(layout.screen());
        layout.debug(box1.outerBox);
        layout.debug(box1.innerBox);
    }

    void draw() {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        shader.attribPointer(posAttrib, VertexBuffer::wrap<Vec2>({ { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } }));
        glDrawArrays(GL_QUADS, 0, 4);
    }
};

}

namespace ui {

void test() {

    sparse::LUDecomposition lu;
    lu.u.appendColumn(sparse::StaticVector({ { 1, 1.0f }, { 3, -0.5f } }).copy());
    lu.u.appendColumn(sparse::StaticVector({ { 0, 0.5f }, { 1, 1.0f } }).copy());
    lu.u.appendColumn(sparse::allocateEmpty());
    lu.u.appendColumn(sparse::StaticVector({ { 0, -0.5f }, { 3, 3.0f } }).copy());
    lu.decompose(4);
    lu.l.debug();
    lu.u.debug();
    float result[4] = {};
    lu.inverseTranspose(std::array<float, 4> { 1.0f, 2.0f, 1000.0f, 3.0f }.data(), result);
    log("result = ({}, {}, {}, {})", result[0], result[1], result[2], result[3]);

    std::unique_ptr<plt::Window> window = plt::Window::construct();
    UiTest callback;
    window->setStateCallback(&callback);
    window->setTitle("UiTest");
    gl::initialize();
    gl::attachWindow(window.get());
    window->initialize();
    while (!window->destoryed()) {
        plt::processEvents();
        callback.draw();
        SwapBuffers(((plt::windows::WindowImpl*)window.get())->deviceContext());
    }
}

}