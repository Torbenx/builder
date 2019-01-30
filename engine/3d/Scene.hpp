#ifndef HEADER_3D_SCENE_H
#define HEADER_3D_SCENE_H

#include "../lgraphic/ShaderBuilder.hpp"

class Scene{
    void addLight(PointLight *light);
    void addLight(SpotLight *light);
    void addLight(AmbientLight *light);
};

#endif