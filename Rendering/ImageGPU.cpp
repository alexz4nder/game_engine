//
// Created by aco on 11/03/2026.
//

#include "ImageGPU.h"

#include "Renderer.h"
#include "../util/logging.h"

void ImageGPU::init(vk::Extent3D size, vk::ImageUsageFlags usage, vk::Format format) {
    this->format = format;
    this->dimension = size;

    vk::ImageCreateInfo imageCreateInfo;
    imageCreateInfo.extent = size;
    imageCreateInfo.usage = usage;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocationCreateInfo;
    allocationCreateInfo.flags = vma::AllocationCreateFlagBits::eDedicatedMemory;
    allocationCreateInfo.usage = vma::MemoryUsage::eGpuOnly;

    vk::Result res = Renderer::allocator.createImage(&imageCreateInfo, &allocationCreateInfo, &image, &allocation,
                                                     &allocationInfo);
    if (res != vk::Result::eSuccess) {
        fnLogg("ERROR");
        abort();
    }
}

void ImageGPU::transferImage(void *img, vk::ImageLayout layout) {
    this->layout = layout;
    vk::Buffer stagingBuffer;
    vma::Allocation stagingBuffAlloc;

    vk::BufferCreateInfo stagingBufferInfo;
    stagingBufferInfo.size = (dimension.height * dimension.width) * 4;
    stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    vma::AllocationCreateInfo stagingAllocInfo;
    stagingAllocInfo.usage = vma::MemoryUsage::eAuto;
    stagingAllocInfo.flags = vma::AllocationCreateFlagBits::eHostAccessRandom;
    stagingAllocInfo.requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible;

    vk::Result res = Renderer::allocator.createBuffer(&stagingBufferInfo, &stagingAllocInfo, &stagingBuffer,
                                                      &stagingBuffAlloc, nullptr);
    if (res != vk::Result::eSuccess) {
        fnLogg("couldnt create staging buffer");
        abort();
    }
    Renderer::allocator.copyMemoryToAllocation(img, stagingBuffAlloc, 0, 4 * (dimension.height * dimension.width));

    vk::BufferImageCopy2 bufferImageCopy;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.imageExtent = dimension;
    bufferImageCopy.imageOffset = vk::Offset3D(0, 0, 0);

    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;

    vk::CopyBufferToImageInfo2 copyInfo;
    copyInfo.dstImage = image;
    copyInfo.srcBuffer = stagingBuffer;
    copyInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    copyInfo.regionCount = 1;
    copyInfo.pRegions = &bufferImageCopy;

    vk::Queue transferQueue = Renderer::vkbDevice.get_dedicated_queue(vkb::QueueType::transfer).value();
    CmdBuffer cmdBuff(vkb::QueueType::transfer);
    vk::CommandBufferSubmitInfo cmdBufferInfo;
    cmdBufferInfo.commandBuffer = cmdBuff.buffer;
    vk::SubmitInfo2 submit;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdBufferInfo;


    cmdBuff.buffer.begin(vk::CommandBufferBeginInfo());
    cmdBuff.transitionImage(image,
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                            vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite,
                            vk::PipelineStageFlagBits::eNone, vk::PipelineStageFlagBits::eTransfer,
                            vk::ImageAspectFlagBits::eColor);
    cmdBuff.buffer.copyBufferToImage2(copyInfo);
    cmdBuff.transitionImage(image,
                            vk::ImageLayout::eTransferDstOptimal, layout,
                            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eNone,
                            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eNone,
                            vk::ImageAspectFlagBits::eColor);
    cmdBuff.buffer.end();
    transferQueue.submit2(submit, nullptr);
    transferQueue.waitIdle();

    Renderer::allocator.destroyBuffer(stagingBuffer, stagingBuffAlloc);
}

void ImageGPU::createView() {
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = image;
    viewInfo.components.r = vk::ComponentSwizzle::eR;
    viewInfo.components.g = vk::ComponentSwizzle::eG;
    viewInfo.components.b = vk::ComponentSwizzle::eB;
    viewInfo.components.a = vk::ComponentSwizzle::eA;
    viewInfo.format = format;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.viewType = vk::ImageViewType::e2D;

    Renderer::device.createImageView(&viewInfo, nullptr, &view);
}
