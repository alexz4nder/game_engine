//
// Created by aco on 11/03/2026.
//

#ifndef GAME_ENGINE_TRANSFORM_H
#define GAME_ENGINE_TRANSFORM_H
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>


class Transform {
public:
    glm::vec3 position={0,0,0};
    glm::quat rotation={1,0,0,0};
    glm::vec3 scale={1,1,1};
    glm::mat4 getTransformMatrix();
};


#endif //GAME_ENGINE_TRANSFORM_H