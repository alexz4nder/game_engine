//
// Created by aco on 09/03/2026.
//


#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#ifndef GAME_ENGINE_SHADEROBJ_H
#define GAME_ENGINE_SHADEROBJ_H


class ShaderObj {
public:
    vk::ShaderEXT shader;

    void load(char *path,vk::ShaderCreateInfoEXT& info);
    void createVertexShader();
    void createFragmentShader();
    void createLightCullingShader();
};


#endif //GAME_ENGINE_SHADEROBJ_H