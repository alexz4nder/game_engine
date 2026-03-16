//
// Created by aco on 12/03/2026.
//

#ifndef GAME_ENGINE_POINTLIGHT_H
#define GAME_ENGINE_POINTLIGHT_H
#include <glm/vec3.hpp>


struct PointLight {
    float intensity;
    glm::vec3 color;
};


#endif //GAME_ENGINE_POINTLIGHT_H