//
// Created by aco on 09/03/2026.
//

#include "ShaderObj.h"

#include <filesystem>
#include <fstream>
#include "Renderer.h"
#include "../util/logging.h"

void ShaderObj::load(char *path,vk::ShaderCreateInfoEXT& info) {
    info.codeSize = std::filesystem::file_size(path);
    info.codeType=vk::ShaderCodeTypeEXT::eSpirv;
    info.pCode=(void*)(new char[info.codeSize]);
    std::ifstream file(path,std::ios::binary);
    file.read((char*)info.pCode,info.codeSize);
    file.close();

}

vk::PushConstantRange range(vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eVertex,0,sizeof(Renderer::PushConstant));
void ShaderObj::createVertexShader() {
    vk::ShaderCreateInfoEXT info;
    load("../shaders/shader.spv",info);

    info.stage=vk::ShaderStageFlagBits::eVertex;
    info.nextStage=vk::ShaderStageFlagBits::eFragment;
    info.pName="vertexShader";

    info.setLayoutCount=sizeof(Renderer::setLayouts)/sizeof(vk::DescriptorSetLayout);
    info.pSetLayouts=Renderer::setLayouts;
    info.pushConstantRangeCount=1;
    info.pPushConstantRanges=&range;


    auto res = Renderer::device.createShaderEXT(info);
    if (!res.has_value()) {
        fnLogg("error");
        abort();
    }
    shader=res.value;
}

void ShaderObj::createFragmentShader() {
    vk::ShaderCreateInfoEXT info;
    load("../shaders/shader.spv",info);

    info.stage=vk::ShaderStageFlagBits::eFragment;
    //info.nextStage=vk::ShaderStageFlagBits::eFragment;
    info.pName="fragmentShader";

    info.setLayoutCount=sizeof(Renderer::setLayouts)/sizeof(vk::DescriptorSetLayout);
    info.pSetLayouts=Renderer::setLayouts;
    info.pushConstantRangeCount=1;
    info.pPushConstantRanges=&range;

    auto res = Renderer::device.createShaderEXT(info);
    if (!res.has_value()) {
        fnLogg("error");
        abort();
    }
    shader=res.value;
}

void ShaderObj::createLightCullingShader() {
    vk::ShaderCreateInfoEXT info;
    load("../shaders/lightCulling.spv",info);

    info.stage=vk::ShaderStageFlagBits::eCompute;
    info.pName="main";

    info.setLayoutCount=sizeof(Renderer::setLayouts)/sizeof(vk::DescriptorSetLayout);
    info.pSetLayouts=Renderer::setLayouts;

    auto res = Renderer::device.createShaderEXT(info);
    if (!res.has_value()) {
        fnLogg("error");
        abort();
    }
    shader=res.value;
}
