//
// Created by aco on 10/03/2026.
//

#ifndef GAME_ENGINE_CAMERA_H
#define GAME_ENGINE_CAMERA_H

#include <glm/glm.hpp>

#include "BufferGPU.h"


struct Camera {
    glm::vec3 position={0,0,0};
    glm::vec3 rotation={0,0,0};


    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    glm::mat4 getViewProjectionMatrix();
};


#endif //GAME_ENGINE_CAMERA_H