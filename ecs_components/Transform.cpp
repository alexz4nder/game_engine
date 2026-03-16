//
// Created by aco on 11/03/2026.
//

#include "Transform.h"

glm::mat4 Transform::getTransformMatrix() {
    glm::mat4 scaleMat={
        scale.x,0,0,0,//
        0,scale.y,0,0,//
        0,0,scale.z,0,//
        0,0,0,1//
    };
    glm::mat4 rotateMat= glm::mat4_cast(rotation);
    glm::mat4 translateMat = glm::mat4(
        1,0,0,0,//
        0,1,0,0,//
        0,0,1,0,//
        position.x,position.y,position.z,1);
    return translateMat*rotateMat*scaleMat;
}
