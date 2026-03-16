//
// Created by aco on 08/03/2026.
//




#include "Renderer.h"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
#include "Vertex.h"

#include "../util/logging.h"
#include <fmt/printf.h>
#include <iostream>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "CmdBuffer.h"

void Renderer::initializeRenderer() {
    SDL_Init(SDL_INIT_VIDEO);
    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    initInstance();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    window=SDL_CreateWindow("game_engine",screenDimension.width,screenDimension.height,SDL_WINDOW_VULKAN);
    if (window==nullptr) {
        fnLogg("WINDOW ERROR");
        abort();
    }
    //SDL_SetWindowResizable(window,true);
    if(!SDL_Vulkan_CreateSurface(window,instance,nullptr,(VkSurfaceKHR*)&surface)){
        fnLogg("SURFACE ERROR");
        abort();
    }
    initPhysicalDevice();
    initDevice();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    initAllocator();
    initSwapchain();
    initDepthBuffer();

    Vertex::initVertexAttribute();
    initImGui();
    initPipelineAndDescriptorLayouts();
    initDescriptorSets();

    fnLogg("SUCCESSFULLY INITIALIZED");
}



static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "[validation layer]" << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
void Renderer::initInstance() {
    vkb::InstanceBuilder builder;
    uint32_t sdlExtensionCount;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
    auto res = builder
    .require_api_version(VK_API_VERSION_1_3)
    .enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
    .request_validation_layers()
    .set_debug_callback(debugCallback)
    //.use_default_debug_messenger()
    .enable_extensions(sdlExtensionCount,sdlExtensions)
    .build();

    if (res.has_value()==false) {
        fnLogg("error");
        abort();
    }
    Renderer::vkbInstance=res.value();
    Renderer::instance=vkbInstance.instance;
}

void Renderer::initPhysicalDevice() {
    vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures;
    descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing=true;
    descriptorIndexingFeatures.runtimeDescriptorArray=true;
    descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind=true;


    vkb::PhysicalDeviceSelector selector(vkbInstance);
    auto res = selector
    .set_surface(surface)
    .set_minimum_version(1,3)
    .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
    .require_present()
    .set_minimum_version(1,3)
    .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
    //.add_required_extension(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME)
    .add_required_extension_features(vk::PhysicalDeviceSynchronization2FeaturesKHR(true))
    .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
    .add_required_extension_features(vk::PhysicalDeviceDynamicRenderingFeaturesKHR(true))
    .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
    .add_required_extension(VK_EXT_SHADER_OBJECT_EXTENSION_NAME)
    .add_required_extension_features(vk::PhysicalDeviceShaderObjectFeaturesEXT(true))
    .add_required_extension(VK_KHR_MAINTENANCE1_EXTENSION_NAME)
    .add_required_extension(VK_KHR_MAINTENANCE_6_EXTENSION_NAME)
    .add_required_extension(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME)
    .add_required_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
    .add_required_extension_features(descriptorIndexingFeatures)
    .select();

    if (res.has_value()==false) {
        fnLogg("{}",res.error().message().c_str());
        abort();
    }

    vkbPhysicalDevice=res.value();
    physicalDevice=vkbPhysicalDevice.physical_device;
    fnLogg("graphics device:{}",vkbPhysicalDevice.name);

}

void Renderer::initDevice() {
    vkb::DeviceBuilder builder(vkbPhysicalDevice);
    auto res = builder.build();
    if (res.has_value()==false) {
        fnLogg("{}",res.error().message());
        abort();
    }
    vkbDevice=res.value();
    device=vkbDevice.device;
}

void Renderer::initAllocator() {
    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.instance=instance;
    allocatorCreateInfo.physicalDevice=physicalDevice;
    allocatorCreateInfo.device=device;
    allocatorCreateInfo.vulkanApiVersion=vk::ApiVersion13;

    vk::Result res = vma::createAllocator(&allocatorCreateInfo,&allocator);

    if (res!=vk::Result::eSuccess) {
        fnLogg("ERROR");
        abort();
    }
    fnLogg("SUCCESS");
}

void Renderer::initSwapchain() {
    vkb::SwapchainBuilder builder(vkbDevice,surface);
    auto res = builder
    .set_desired_extent(screenDimension.width,screenDimension.height)
    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
    .set_required_min_image_count(3)
    .build();
    if (res.has_value()==false) {
        fnLogg("{}",res.error().message());
        abort();
    }
    swapchain=res->swapchain;
    screenDimension=res->extent;
    swapchainFormat=(vk::Format)res->image_format;

    //swapchain images and views
    uint32_t swapchainImageCount;
    device.getSwapchainImagesKHR(swapchain,&swapchainImageCount,nullptr);
    device.getSwapchainImagesKHR(swapchain,&swapchainImageCount,swapchainImages);

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.format=swapchainFormat;
    viewInfo.viewType=vk::ImageViewType::e2D;
    viewInfo.subresourceRange.aspectMask=vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.levelCount=1;
    viewInfo.subresourceRange.layerCount=1;
    for (int i =0;i<3;i++) {
        viewInfo.image=swapchainImages[i];
        device.createImageView(&viewInfo,nullptr,&(swapchainImageViews[i]));
    }

    //swapchainFence
    vk::FenceCreateInfo fenceInfo;
    //fenceInfo.flags=vk::FenceCreateFlags::BitsType::eSignaled;
    device.createFence(&fenceInfo,nullptr,&swapchainFence);

    fnLogg("image count:{}",res->image_count);
}

void Renderer::initDepthBuffer() {
    depthBuffer.init(vk::Extent3D(screenDimension,1),vk::ImageUsageFlagBits::eDepthStencilAttachment,vk::Format::eD32Sfloat);
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image=depthBuffer.image;
    viewInfo.components.r=vk::ComponentSwizzle::eR;
    viewInfo.components.g=vk::ComponentSwizzle::eG;
    viewInfo.components.b=vk::ComponentSwizzle::eB;
    viewInfo.components.a=vk::ComponentSwizzle::eA;
    viewInfo.format=vk::Format::eD32Sfloat;
    viewInfo.subresourceRange.levelCount=1;
    viewInfo.subresourceRange.baseMipLevel=0;
    viewInfo.subresourceRange.layerCount=1;
    viewInfo.subresourceRange.baseArrayLayer=0;
    viewInfo.subresourceRange.aspectMask=vk::ImageAspectFlagBits::eDepth;
    viewInfo.viewType=vk::ImageViewType::e2D;

    Renderer::device.createImageView(&viewInfo,nullptr,&depthBuffer.view);
}

void Renderer::initImGui() {
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_DockingEnable;
    if (!ImGui_ImplSDL3_InitForVulkan(window)) {
        fnLogg("SDL3 error");
        abort();
    }



    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.poolSizeCount=11;
    poolCreateInfo.pPoolSizes=(vk::DescriptorPoolSize*)poolSizes;
    poolCreateInfo.maxSets=1000;
    poolCreateInfo.flags=vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    device.createDescriptorPool(&poolCreateInfo,nullptr,&imGuiPool);

    ImGui_ImplVulkan_InitInfo initInfo={0};
    initInfo.Instance=instance;
    initInfo.PhysicalDevice=physicalDevice;
    initInfo.Device=device;
    initInfo.QueueFamily=ImGui_ImplVulkanH_SelectQueueFamilyIndex(physicalDevice);
    initInfo.Queue=vkbDevice.get_queue(vkb::QueueType::graphics).value();
    initInfo.PipelineCache=nullptr;
    initInfo.DescriptorPool=imGuiPool;
    //initInfo.DescriptorPoolSize=1000;
    initInfo.MinImageCount=3;
    initInfo.ImageCount=3;
    initInfo.UseDynamicRendering=true;

    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount=1;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats=(VkFormat*)&swapchainFormat;
    initInfo.PipelineInfoMain.SwapChainImageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    initInfo.PipelineInfoMain.MSAASamples=VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        fnLogg("vulkan init error");
        abort();
    }

    imguiCmd=CmdBuffer(vkb::QueueType::graphics);
}


void Renderer::initPipelineAndDescriptorLayouts() {
    ///Descriptor pool
    vk::DescriptorPoolSize poolSizes[]={
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer,2),
        vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage,1024),
        vk::DescriptorPoolSize(vk::DescriptorType::eSampler,2),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer,2)
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.maxSets=3;
    poolCreateInfo.flags=vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
    poolCreateInfo.poolSizeCount=sizeof(poolSizes)/sizeof(vk::DescriptorPoolSize);
    poolCreateInfo.pPoolSizes=poolSizes;
    device.createDescriptorPool(&poolCreateInfo,nullptr,&descriptorPool);

    //////////////////
    ///SET LAYOUTS///
    /////////////////////////////////////////
    ///descriptor set 0 - constant variables
    ///descriptor set 1 - frame constant variables
    ///descriptor set 2 - descriptor indexing

    //set 0
    vk::DescriptorSetLayoutBinding set0Bindings[]={
        vk::DescriptorSetLayoutBinding(0,vk::DescriptorType::eSampler,1,
            vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eVertex)

    };
    vk::DescriptorSetLayoutCreateInfo set0LayoutInfo;
    //set0LayoutInfo.flags=
    set0LayoutInfo.bindingCount=sizeof(set0Bindings)/sizeof(vk::DescriptorSetLayoutBinding);
    set0LayoutInfo.pBindings=set0Bindings;


    //set 1
    vk::DescriptorSetLayoutBinding set1Bindings[]={
        vk::DescriptorSetLayoutBinding(0,vk::DescriptorType::eUniformBuffer,1,
            vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eCompute),
        vk::DescriptorSetLayoutBinding(1,vk::DescriptorType::eUniformBuffer,1,
            vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eCompute),
        vk::DescriptorSetLayoutBinding(2,vk::DescriptorType::eStorageBuffer,1,
            vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eCompute),
        vk::DescriptorSetLayoutBinding(3,vk::DescriptorType::eStorageBuffer,1,
            vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eCompute)
    };
    vk::DescriptorSetLayoutCreateInfo set1LayoutInfo;
    //set1LayoutInfo.flags=
    set1LayoutInfo.bindingCount=sizeof(set1Bindings)/sizeof(vk::DescriptorSetLayoutBinding);
    set1LayoutInfo.pBindings=set1Bindings;

    //set 2
    vk::DescriptorSetLayoutBinding set2Bindings[]={
        vk::DescriptorSetLayoutBinding(0,vk::DescriptorType::eSampledImage,1024,
            vk::ShaderStageFlagBits::eFragment),
        vk::DescriptorSetLayoutBinding(1,vk::DescriptorType::eSampler,1,
            vk::ShaderStageFlagBits::eFragment)
    };
    vk::DescriptorSetLayoutCreateInfo set2LayoutInfo;
    set2LayoutInfo.flags=vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
    set2LayoutInfo.bindingCount=sizeof(set2Bindings)/sizeof(vk::DescriptorSetLayoutBinding);
    set2LayoutInfo.pBindings=set2Bindings;



    device.createDescriptorSetLayout(&set0LayoutInfo,nullptr,&setLayouts[0]);
    device.createDescriptorSetLayout(&set1LayoutInfo,nullptr,&setLayouts[1]);
    device.createDescriptorSetLayout(&set2LayoutInfo,nullptr,&setLayouts[2]);

    ///Push constants
    vk::PushConstantRange pushConstantRange;
    pushConstantRange.stageFlags=vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eVertex;
    pushConstantRange.offset=0;
    pushConstantRange.size=sizeof(PushConstant);


    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.pushConstantRangeCount=1;
    layoutCreateInfo.pPushConstantRanges=&pushConstantRange;
    layoutCreateInfo.setLayoutCount=3;
    layoutCreateInfo.pSetLayouts=setLayouts;

    vk::PipelineLayoutCreateInfo computeLayoutCreateInfo;
    computeLayoutCreateInfo.setLayoutCount=3;
    computeLayoutCreateInfo.pSetLayouts=setLayouts;

    device.createPipelineLayout(&layoutCreateInfo,nullptr,&pipelineLayout);
    device.createPipelineLayout(&computeLayoutCreateInfo,nullptr,&computeLayout);
}

void Renderer::initDescriptorSets() {
    frameConstantsGPU.init(sizeof(FrameConstants),vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst);
    //init buffers for lights
    lightBuffer.init(sizeof(LightData),vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst);
    int numberOfClusters=32*8*12;
    clusterListGPU.init(sizeof(ClusterList)*numberOfClusters,vk::BufferUsageFlagBits::eStorageBuffer);
    activeLights.init(sizeof(ActiveLightsList),vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst);



    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.addressModeU=vk::SamplerAddressMode::eClampToBorder;
    samplerInfo.addressModeV=vk::SamplerAddressMode::eClampToBorder;
    samplerInfo.addressModeW=vk::SamplerAddressMode::eClampToBorder;
    samplerInfo.magFilter=vk::Filter::eNearest;
    samplerInfo.minFilter=vk::Filter::eNearest;
    samplerInfo.mipmapMode=vk::SamplerMipmapMode::eNearest;
    samplerInfo.anisotropyEnable=false;
    samplerInfo.borderColor=vk::BorderColor::eFloatOpaqueWhite;
    samplerInfo.unnormalizedCoordinates=vk::False;
    samplerInfo.compareEnable=vk::False;
    device.createSampler(&samplerInfo,nullptr,&sampler);



    vk::DescriptorSetAllocateInfo setInfo;
    setInfo.descriptorPool=descriptorPool;
    setInfo.pSetLayouts=setLayouts;
    setInfo.descriptorSetCount=3;
    device.allocateDescriptorSets(&setInfo,descriptorSets);


    //WRITE SET 1
    vk::DescriptorBufferInfo frameConstantBufferInfo;
    frameConstantBufferInfo.buffer=frameConstantsGPU.buffer;
    frameConstantBufferInfo.offset=0;
    frameConstantBufferInfo.range=sizeof(FrameConstants);
    vk::WriteDescriptorSet writeFrameConstants;
    writeFrameConstants.dstSet=descriptorSets[1];
    writeFrameConstants.dstBinding=0;
    writeFrameConstants.descriptorCount=1;
    writeFrameConstants.descriptorType=vk::DescriptorType::eUniformBuffer;
    writeFrameConstants.pBufferInfo=&frameConstantBufferInfo;
    device.updateDescriptorSets(1,&writeFrameConstants,0,nullptr);

    vk::WriteDescriptorSet writeLights[3];
    vk::DescriptorBufferInfo lightsBufferInfo[3];

    lightsBufferInfo[0].buffer=lightBuffer.buffer;
    lightsBufferInfo[0].offset=0;
    lightsBufferInfo[0].range=sizeof(LightData);
    writeLights[0].dstSet=descriptorSets[1];
    writeLights[0].dstBinding=1;
    writeLights[0].descriptorCount=1;
    writeLights[0].descriptorType=vk::DescriptorType::eUniformBuffer;
    writeLights[0].pBufferInfo=&lightsBufferInfo[0];

    lightsBufferInfo[1].buffer=clusterListGPU.buffer;
    lightsBufferInfo[1].offset=0;
    lightsBufferInfo[1].range=sizeof(ClusterList)*32*8*12;
    writeLights[1].dstSet=descriptorSets[1];
    writeLights[1].dstBinding=2;
    writeLights[1].descriptorCount=1;
    writeLights[1].descriptorType=vk::DescriptorType::eStorageBuffer;
    writeLights[1].pBufferInfo=&lightsBufferInfo[1];

    lightsBufferInfo[2].buffer=activeLights.buffer;
    lightsBufferInfo[2].offset=0;
    lightsBufferInfo[2].range=sizeof(ActiveLightsList);
    writeLights[2].dstSet=descriptorSets[1];
    writeLights[2].dstBinding=3;
    writeLights[2].descriptorCount=1;
    writeLights[2].descriptorType=vk::DescriptorType::eStorageBuffer;
    writeLights[2].pBufferInfo=&lightsBufferInfo[2];

    device.updateDescriptorSets(3,writeLights,0,nullptr);
    //WRITE SET 2
    vk::DescriptorImageInfo samplerImageInfo;
    samplerImageInfo.sampler=sampler;
    vk::WriteDescriptorSet writeSampler;
    writeSampler.descriptorCount=1;
    writeSampler.descriptorType=vk::DescriptorType::eSampler;
    writeSampler.dstBinding=1;
    writeSampler.dstSet=descriptorSets[2];
    writeSampler.pImageInfo = &samplerImageInfo;
    writeSampler.dstArrayElement=0;
    device.updateDescriptorSets(1,&writeSampler,0,nullptr);
}
