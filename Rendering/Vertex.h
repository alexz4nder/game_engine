//
// Created by aco on 10/03/2026.
//

#ifndef GAME_ENGINE_VERTEX_H
#define GAME_ENGINE_VERTEX_H
#include <glm/glm.hpp>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 biTangent;
    glm::vec2 texCoord;

    inline static vk::VertexInputAttributeDescription2EXT attributeDescription[5];
    inline static vk::VertexInputBindingDescription2EXT bindingDescription;
    static void initVertexAttribute();
};


#endif //GAME_ENGINE_VERTEX_H