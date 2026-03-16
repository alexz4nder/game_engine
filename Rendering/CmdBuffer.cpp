//
// Created by aco on 09/03/2026.
//

#include "CmdBuffer.h"

#include "Renderer.h"
#include "../ecs_components/Mesh.h"
#include "Renderer.h"

CmdBuffer::CmdBuffer() {
}

CmdBuffer::CmdBuffer(vkb::QueueType queueType) {
    vk::CommandPoolCreateInfo poolCreateInfo;
    poolCreateInfo.queueFamilyIndex = Renderer::vkbDevice.get_queue_index(queueType).value();
    poolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    Renderer::device.createCommandPool(&poolCreateInfo, nullptr, &pool);

    vk::CommandBufferAllocateInfo bufferInfo;
    bufferInfo.commandBufferCount = 1;
    bufferInfo.level = vk::CommandBufferLevel::ePrimary;
    bufferInfo.commandPool = pool;
    Renderer::device.allocateCommandBuffers(&bufferInfo, &buffer);
}

void CmdBuffer::beginRendering(vk::ImageView target, vk::ImageView depthBuffer) {
    vk::RenderingAttachmentInfo depthAttachment;
    vk::RenderingAttachmentInfo colorAttachment;
    colorAttachment.imageView = target;
    colorAttachment.imageLayout = vk::ImageLayout::eAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;


    vk::RenderingInfoKHR renderingInfo;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.renderArea = vk::Rect2D({0, 0}, Renderer::screenDimension);
    renderingInfo.layerCount = 1;
    if (depthBuffer != nullptr) {
        renderingInfo.pDepthAttachment = &depthAttachment;
        depthAttachment.imageLayout = vk::ImageLayout::eUndefined;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        depthAttachment.imageView = depthBuffer;
        depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    }
    buffer.beginRenderingKHR(renderingInfo);

    buffer.setScissorWithCount(vk::Rect2D({0, 0}, Renderer::screenDimension));
    buffer.setViewportWithCount(vk::Viewport(0, float(Renderer::screenDimension.height),
                                             Renderer::screenDimension.width, -float(Renderer::screenDimension.height),
                                             0, 1));
}

void CmdBuffer::clearImage(vk::ImageView target, vk::ClearValue clearColor, vk::ImageView depthBuffer) {
    vk::RenderingAttachmentInfo depthAttchment;
    vk::RenderingAttachmentInfo colorAttachment;
    colorAttachment.imageView = target;
    colorAttachment.imageLayout = vk::ImageLayout::eAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.clearValue = vk::ClearValue(clearColor);
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;


    vk::RenderingInfoKHR renderingInfo;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.renderArea = vk::Rect2D({0, 0}, Renderer::screenDimension);
    renderingInfo.layerCount = 1;

    if (depthBuffer != nullptr) {
        renderingInfo.pDepthAttachment = &depthAttchment;
        depthAttchment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        depthAttchment.imageView = depthBuffer;
        depthAttchment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttchment.clearValue = vk::ClearDepthStencilValue(1.0f, 0);
    }

    buffer.beginRenderingKHR(renderingInfo);
    buffer.endRenderingKHR();
}

void CmdBuffer::stupidBoilerPlate() {
    //////////////////////////
    ///TO BE DELETED LATER///
    ////////////////////////
    buffer.setDepthTestEnable(false);

    ////////////////////////
    buffer.setRasterizerDiscardEnable(false);
    buffer.setCullMode(vk::CullModeFlagBits::eNone);
    buffer.setPolygonModeEXT(vk::PolygonMode::eFill);
    buffer.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);
    buffer.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
    buffer.setStencilTestEnable(false);


    ///////
    buffer.setAlphaToCoverageEnableEXT(false);
    buffer.setDepthBiasEnable(false);
    vk::Bool32 b = false;
    buffer.setColorBlendEnableEXT(0, 1, &b);
    buffer.setPrimitiveRestartEnable(false);
    vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR |
                                             vk::ColorComponentFlagBits::eG |
                                             vk::ColorComponentFlagBits::eB |
                                             vk::ColorComponentFlagBits::eA;
    buffer.setColorWriteMaskEXT(0, 1, &colorWriteMask);
    vk::SampleMask sampleMask = 1;
    buffer.setSampleMaskEXT(vk::SampleCountFlagBits::e1, &sampleMask);
}

void CmdBuffer::drawMesh(Mesh &mesh, Transform &transform) {
    if (mesh.geometryIndex == -1) {
        return;
    }
    MeshGeometry &geometry = Renderer::meshGeometries[mesh.geometryIndex];
    Renderer::pushConstant.transform = transform.getTransformMatrix();

    if (mesh.materialIndex == -1) {
        Renderer::pushConstant.colorMap = -1;
        Renderer::pushConstant.normalMap = -1;
    } else {
        Material &material = Renderer::materials[mesh.materialIndex];
        Renderer::pushConstant.colorMap = material.colorIndex;
        Renderer::pushConstant.normalMap = material.normalIndex;
    }

    buffer.pushConstants(Renderer::pipelineLayout,
                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                         0, sizeof(Renderer::PushConstant), &Renderer::pushConstant);

    vk::DeviceSize offsets = 0;
    buffer.bindVertexBuffers(0, 1, &geometry.vertexBuffer.buffer, &offsets);
    buffer.bindIndexBuffer(geometry.indexBuffer.buffer, 0, vk::IndexType::eUint32);

    buffer.drawIndexed(geometry.numberOfIndices, 1, 0, 0, 0);
}


void CmdBuffer::transitionImage(vk::Image image,
                                vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage,
                                uint32_t srcQueue, uint32_t dstQueue,
                                vk::ImageAspectFlags aspectFlags) {
    vk::ImageMemoryBarrier2 imageMemoryBarrier;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.subresourceRange.aspectMask = aspectFlags;
    imageMemoryBarrier.srcAccessMask = srcAccessMask;
    imageMemoryBarrier.dstAccessMask = dstAccessMask;
    imageMemoryBarrier.srcStageMask = srcStage;
    imageMemoryBarrier.dstStageMask = dstStage;
    imageMemoryBarrier.srcQueueFamilyIndex = srcQueue;
    imageMemoryBarrier.dstQueueFamilyIndex = dstQueue;


    vk::DependencyInfoKHR dependencyInfo;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageMemoryBarrier;

    buffer.pipelineBarrier2(dependencyInfo);
}

void CmdBuffer::transitionImage(vk::Image image,
                                vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                                vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage,
                                vk::ImageAspectFlags aspectFlags) {

    vk::ImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.image=image;
    imageMemoryBarrier.oldLayout=oldLayout;
    imageMemoryBarrier.newLayout=newLayout;
    imageMemoryBarrier.subresourceRange.levelCount=1;
    imageMemoryBarrier.subresourceRange.layerCount=1;
    imageMemoryBarrier.subresourceRange.aspectMask=aspectFlags;
    imageMemoryBarrier.srcAccessMask=srcAccessMask;
    imageMemoryBarrier.dstAccessMask=dstAccessMask;

    buffer.pipelineBarrier(srcStage,dstStage,vk::DependencyFlags(),0,nullptr,0,nullptr,1,&imageMemoryBarrier);
}

void CmdBuffer::enableDepth() {
    buffer.setDepthTestEnable(true);
    buffer.setDepthCompareOp(vk::CompareOp::eLess);
    buffer.setDepthWriteEnable(true);
}
