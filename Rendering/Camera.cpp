//
// Created by aco on 10/03/2026.
//

#include "Camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "Renderer.h"


glm::mat4 Camera::getViewProjectionMatrix() {
    return getProjectionMatrix() * getViewMatrix();
}

glm::mat4 Camera::getProjectionMatrix() {
    return glm::perspectiveLH(glm::radians(60.0),
        double(Renderer::screenDimension.width)/double(Renderer::screenDimension.height),
        0.01,500.0);
}

glm::mat4 Camera::getViewMatrix() {
    //glm::vec3 rot = glm::rotateX(glm::vec3(0,0,1),glm::radians(rotation.x));
    //rot=glm::rotateY(rot,glm::radians(rotation.y));
    //return glm::lookAtLH(position,rot,glm::normalize(glm::cross(glm::vec3(0,1,0),rot)));

    glm::mat4 rotationMatrix = glm::eulerAngleXY(glm::radians(rotation.x),glm::radians(rotation.y));
    glm::mat4 translationMatrix = glm::mat4(
        1,0,0,0,//
        0,1,0,0,//
        0,0,1,0,//
        -position.x,-position.y,-position.z,1);



    return rotationMatrix*translationMatrix;
}