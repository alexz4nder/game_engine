//
// Created by aco on 09/03/2026.
//

#include "BufferGPU.h"
#include "Renderer.h"
#include "../util/logging.h"

void BufferGPU::init(uint32_t size,vk::BufferUsageFlags usage) {
    this->size=size;

    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.size=size;
    bufferCreateInfo.usage=usage;

    vma::AllocationCreateInfo allocationCreateInfo;
    allocationCreateInfo.flags=vma::AllocationCreateFlags::BitsType::eDedicatedMemory;
    allocationCreateInfo.usage=vma::MemoryUsage::eGpuOnly;

    Renderer::allocator.createBuffer(&bufferCreateInfo,&allocationCreateInfo,&buffer,&allocation,&allocInfo);
}

void BufferGPU::dataTransfer(void *data, uint32_t size) {
    vk::Buffer stagingBuffer;
    vma::Allocation stagingBuffAlloc;

    vk::BufferCreateInfo stagingBufferInfo;
    stagingBufferInfo.size=size;
    stagingBufferInfo.usage=vk::BufferUsageFlagBits::eTransferSrc;
    vma::AllocationCreateInfo stagingAllocInfo;
    stagingAllocInfo.usage=vma::MemoryUsage::eAuto;
    stagingAllocInfo.flags=vma::AllocationCreateFlagBits::eHostAccessRandom;
    stagingAllocInfo.requiredFlags=vk::MemoryPropertyFlagBits::eHostVisible;

    vk::Result res = Renderer::allocator.createBuffer(&stagingBufferInfo,&stagingAllocInfo,&stagingBuffer,&stagingBuffAlloc,nullptr);
    if (res!=vk::Result::eSuccess) {
        fnLogg("couldnt create staging buffer");
        abort();
    }
    Renderer::allocator.copyMemoryToAllocation(data,stagingBuffAlloc,0,size);

    vk::Queue transferQueue = Renderer::vkbDevice.get_dedicated_queue(vkb::QueueType::transfer).value();
    vk::BufferCopy2 copy;
    copy.size=size;
    copy.dstOffset=0;
    copy.srcOffset=0;
    vk::CopyBufferInfo2 copyInfo;
    copyInfo.dstBuffer=buffer;
    copyInfo.srcBuffer=stagingBuffer;
    copyInfo.regionCount=1;
    copyInfo.pRegions=&copy;
    CmdBuffer cmdBuff(vkb::QueueType::transfer);
    cmdBuff.buffer.begin(vk::CommandBufferBeginInfo());
    cmdBuff.buffer.copyBuffer2(copyInfo);
    cmdBuff.buffer.end();

    vk::CommandBufferSubmitInfo cmdBufferInfo;
    cmdBufferInfo.commandBuffer=cmdBuff.buffer;
    vk::SubmitInfo2 submit;
    submit.commandBufferInfoCount=1;
    submit.pCommandBufferInfos=&cmdBufferInfo;
    transferQueue.submit2(submit,nullptr);
    transferQueue.waitIdle();

    Renderer::allocator.destroyBuffer(stagingBuffer,stagingBuffAlloc);
}
