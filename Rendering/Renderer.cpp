//
// Created by aco on 08/03/2026.
//

#include "Renderer.h"
#include "../util/logging.h"

void Renderer::resizeSwapchain() {
    device.waitIdle();
    device.destroySwapchainKHR(swapchain);

    vkb::SwapchainBuilder builder(vkbDevice,surface);
    auto res =builder
    .build();

    if (res.has_value()==false) {
        fnLogg("ERROR");
        abort();
    }
    screenDimension=res->extent;
    swapchain=res.value().swapchain;

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

    //swapchain fence
    device.destroyFence(swapchainFence,nullptr);
    vk::FenceCreateInfo fenceCreateInfo;
    //fenceCreateInfo.flags=vk::FenceCreateFlagBits::eSignaled;
    device.createFence(&fenceCreateInfo,nullptr,&swapchainFence);

    fnLogg("Recreated swapchain");
}


