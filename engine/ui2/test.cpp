#include "GL/Shader.hpp"
#include "Layout.hpp"
#include "opengl/core.hpp"
#include "platform/Window.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "platform/windows/glcore.hpp"
#include <memory>
#include <sstream>

namespace ui {

struct UiTest : plt::WindowStateCallback {
    Shader shader;
    Shader::LocationHandle posAttrib;
    void onInitialize(plt::Window*) override {
        ShaderBuilder builder;
        posAttrib = builder.addAttrib("vec4", "a_position");
        builder.vMain << "gl_Position = a_position;\n";
        builder.fMain << Shader::FRAGMENT_COLOR << " = vec4(1.0, 1.0, 1.0, 1.0);\n";
        shader.initialize(builder);
    }

    void draw() {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        shader.attribPointer(posAttrib, VertexBuffer::wrap<Vec2>({ { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } }));
        glDrawArrays(GL_QUADS, 0, 4);
    }
};

void test() {

    sparse::LUDecomposition lu;
    lu.u.appendColumn(sparse::StaticVector({ { 1, 1.0f }, { 2, -0.5f } }).copy());
    lu.u.appendColumn(sparse::StaticVector({ { 0, 0.5f }, { 1, 1.0f } }).copy());
    lu.u.appendColumn(sparse::StaticVector({ { 0, -0.5f }, { 2, 3.0f } }).copy());
    lu.decompose(3);
    lu.l.debug();
    lu.u.debug();
    sparse::Matrix check = sparse::add(sparse::multiply(lu.l, lu.u), lu.u);
    check.debug();
    float result[3];
    lu.inverseTranspose(std::array<float, 3> { 1.0f, 2.0f, 3.0f }.data(), result);
    log("result = ({}, {}, {})", result[0], result[1], result[2]);

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