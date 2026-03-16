//
// Created by aco on 09/03/2026.
//

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>


#ifndef GAME_ENGINE_BUFFERGPU_H
#define GAME_ENGINE_BUFFERGPU_H

struct BufferGPU {
    vk::Buffer buffer;
    vma::Allocation allocation;
    vma::AllocationInfo allocInfo;
    uint32_t size;

    void init(uint32_t size,vk::BufferUsageFlags flags);
    void dataTransfer(void *data,uint32_t size);
};


#endif //GAME_ENGINE_BUFFERGPU_H