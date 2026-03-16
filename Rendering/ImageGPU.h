//
// Created by aco on 11/03/2026.
//

#ifndef GAME_ENGINE_IMAGEGPU_H
#define GAME_ENGINE_IMAGEGPU_H
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"

class ImageGPU {
public:
    vk::Image image;
    vk::ImageView view;
    vma::Allocation allocation;
    vma::AllocationInfo allocationInfo;
    vk::Extent3D dimension;
    vk::Format format;
    vk::ImageLayout layout;
    char *name;
    void init(vk::Extent3D size,vk::ImageUsageFlags usage,vk::Format format);
    void transferImage(void *img, vk::ImageLayout layout);
    void createView();
};


#endif //GAME_ENGINE_IMAGEGPU_H