//
// Created by aco on 08/03/2026.
//


#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "Camera.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.hpp"
#include "CmdBuffer.h"
#include "ImageGPU.h"
#include "Material.h"
#include "MeshGeometry.h"

#ifndef GAME_ENGINE_RENDERER_H
#define GAME_ENGINE_RENDERER_H


class Renderer {
    friend CmdBuffer;
    friend Camera;

public:
    inline static vk::Extent2D screenDimension = {1080,720};
    ////Vulkan context

    static void initializeRenderer();//Init functions are in RendererInit.cpp

    static void initInstance();
    inline static vkb::Instance vkbInstance;
    inline static vk::Instance instance;

    inline static SDL_Window* window;
    inline static vk::SurfaceKHR surface;

    static void initPhysicalDevice();
    inline static vkb::PhysicalDevice vkbPhysicalDevice;
    inline static vk::PhysicalDevice physicalDevice;

    static void initDevice();
    inline static vkb::Device vkbDevice;
    inline static vk::Device device;

    static void initAllocator();
    inline static vma::Allocator allocator;

    static void initImGui();
    inline static vk::DescriptorPool imGuiPool;
    inline static CmdBuffer imguiCmd;

///Swapchain
    static void initSwapchain();
    inline static vk::SwapchainKHR swapchain;
    inline static vk::Image swapchainImages[3];
    inline static vk::ImageView swapchainImageViews[3];
    inline static vk::Fence swapchainFence;
    inline static vk::Fence finishedRenderingFence;
    inline static uint32_t imageIndex;
    inline static vk::Format swapchainFormat;
    static void resizeSwapchain();

    inline static ImageGPU depthBuffer;
    static void initDepthBuffer();

    //Interfacing with shaders
    static void initPipelineAndDescriptorLayouts();
    inline static vk::DescriptorPool descriptorPool;
    inline static vk::DescriptorSetLayout setLayouts[3];
    inline static vk::DescriptorSet descriptorSets[3];
    inline static vk::PipelineLayout pipelineLayout;
    inline static vk::PipelineLayout computeLayout;
    //Push constant
    inline static struct PushConstant {
        glm::mat4 transform;
        uint32_t colorMap;
        uint32_t normalMap;
    }pushConstant;
    ///Frame constants - set 1
    inline static Camera camera;
    inline static struct FrameConstants {
        struct CameraFrameConstants {
            glm::mat4 viewPerspective;
            glm::mat4 inversePerspective;
            glm::mat4 viewMatrix;
            glm::vec4 cameraPos;
        }camera;
    }frameConstants;
    inline static BufferGPU frameConstantsGPU;
    ///////////////
    /// LIGHTS ///
    /////////////
    struct LightGPU {
        glm::vec3 color;
        float intensity;
        glm::vec3 position;
        uint32_t padding;
    };
    inline static struct LightData {
        uint32_t numberOfLights;
        uint32_t padding[3];
        LightGPU lights[1024];
    }lightData;
    inline static BufferGPU lightBuffer;

    struct ClusterList {
        uint32_t numberOfLights;
        uint32_t firstLight;
    };
    inline static BufferGPU clusterListGPU;

    inline static const uint32_t maxActiveLights=32*8*12*8;
    struct ActiveLightsList {
        uint32_t numberOfLights;
        uint32_t activeLightsIndices[maxActiveLights];
    };
    inline static BufferGPU activeLights;


    //Bindless resources - set 2
    inline static ImageGPU textures[1024];
    inline static vk::Sampler sampler;


    static void initDescriptorSets();


    inline static MeshGeometry meshGeometries[1024];
    inline static Material materials[1024];



    static void mainLoop();
};


#endif //GAME_ENGINE_RENDERER_H