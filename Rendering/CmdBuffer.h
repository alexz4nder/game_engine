//
// Created by aco on 09/03/2026.
//
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>

#include "../ecs_components/Transform.h"
#ifndef GAME_ENGINE_CMDBUFFER_H
#define GAME_ENGINE_CMDBUFFER_H
class Renderer;
struct Mesh;

class CmdBuffer {
public:
    vk::CommandPool pool;
    vk::CommandBuffer buffer;
    friend Renderer;

public:
    CmdBuffer(vkb::QueueType queueType);

    void beginRendering(vk::ImageView target, vk::ImageView depthBuffer = nullptr);

    void clearImage(vk::ImageView target, vk::ClearValue clearColor, vk::ImageView depthBuffer = nullptr);

    void stupidBoilerPlate();

    void drawMesh(Mesh &mesh, Transform &transform);

    void transitionImage(vk::Image image,
                         vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                         vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                         vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage,
                         uint32_t srcQueue, uint32_t dstQueue,
                         vk::ImageAspectFlags aspectFlags);

    void transitionImage(vk::Image image,
                         vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                         vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                         vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage,
                         vk::ImageAspectFlags aspectFlags);

    void enableDepth();

    CmdBuffer();
};


#endif //GAME_ENGINE_CMDBUFFER_H
