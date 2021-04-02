#include "util/draw_util.hpp"
#include "GL/Shader.hpp"

Shader utilCircleShader(
    "attribute vec4 a_position;\n"
    "uniform vec2 u_radius;\n"
    "uniform vec2 u_center;\n"
    "varying vec2 v_pos;\n"
    "void main(){\n"
    "   v_pos = (a_position.xy - u_center) / u_radius;\n"
    "   gl_Position = a_position;\n"
    "}\n",
    "uniform vec4 u_color;\n"
    "varying vec2 v_pos;\n"
    "void main(){\n"
    "   if(dot(v_pos, v_pos) < 1.0){\n"
    "       gl_FragColor = u_color;\n"
    "   }else{\n"
    "       discard;\n"
    "   }\n"
    "}"
);

void util_drawCircle(Vec2 center, Vec2 radius, Vec4 color){
    utilCircleShader.init();
    utilCircleShader.use();
    float temp[2*4] = {center.x-radius.x, center.y-radius.y, center.x+radius.x, center.y-radius.y, center.x-radius.x, center.y+radius.y, center.x+radius.x, center.y+radius.y};
    utilCircleShader.attribPointer("a_position", 2, temp);
    utilCircleShader.uniformVec4("u_color", color);
    utilCircleShader.uniformVec2("u_center", center);
    utilCircleShader.uniformVec2("u_radius", radius);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
