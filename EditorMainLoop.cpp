//
// Created by aco on 08/03/2026.
//


#define GLM_ENABLE_EXPERIMENTAL
#include <filesystem>
#include <glm/gtx/euler_angles.hpp>

#include "Rendering/BufferGPU.h"
#include "flecs.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "Rendering/Renderer.h"
#include "Rendering/ShaderObj.h"
#include "ecs_components/Transform.h"
#include "Rendering/Vertex.h"
#include "InputManagment/Input.h"
#include "util/logging.h"
#include "ecs_components/Mesh.h"
#include <stb/stb_image.h>

#include "ecs_components/PointLight.h"

void loadScene(std::filesystem::path path);

void proccessInput();

void editorDrawing(CmdBuffer &cmd);

void lightTransfer();

void cullLights();

ShaderObj vertextShader;
ShaderObj fragmentShader;
ShaderObj lightCullingShader;
vk::Queue grQueue;
vk::Queue transferQueue;
vk::Queue computeQueue;

extern flecs::world world;
flecs::entity selectedEntity;

void editorMainLoop() {
    selectedEntity = world.entity(0);
    CmdBuffer cmd = CmdBuffer(vkb::QueueType::graphics);

    SDL_SetWindowResizable(Renderer::window, true);
    vk::Queue presentQueue = Renderer::vkbDevice.get_queue(vkb::QueueType::present).value();
    vk::FenceCreateInfo finishedRenderingFenceInfo;
    Renderer::device.createFence(&finishedRenderingFenceInfo, nullptr, &Renderer::finishedRenderingFence);


    loadScene("../models/testScene/testScene.gltf");

    vertextShader.createVertexShader();
    fragmentShader.createFragmentShader();
    lightCullingShader.createLightCullingShader();

    grQueue = Renderer::vkbDevice.get_queue(vkb::QueueType::graphics).value();
    transferQueue = Renderer::vkbDevice.get_dedicated_queue(vkb::QueueType::transfer).value();
    //computeQueue=vkbDevice.get_dedicated_queue(vkb::QueueType::compute).value();


    while (1) {
        /////////////////////////////
        ///ACQUIRE FROM SWAPCHAIN///
        ///////////////////////////
        vk::Result acquireSuccessful = Renderer::device.acquireNextImageKHR(
            Renderer::swapchain,UINT64_MAX, nullptr, Renderer::swapchainFence,
            &Renderer::imageIndex);
        if (acquireSuccessful != vk::Result::eSuccess) {
            Renderer::resizeSwapchain();
            continue;
        }
        Renderer::device.waitForFences(1, &Renderer::swapchainFence, true,UINT64_MAX);
        Renderer::device.resetFences(Renderer::swapchainFence);
        Renderer::device.resetFences(1, &Renderer::finishedRenderingFence);

        //upadate frame constants
        Renderer::frameConstants.camera.viewPerspective = Renderer::camera.getViewProjectionMatrix();
        Renderer::frameConstants.camera.inversePerspective = glm::inverse(Renderer::camera.getProjectionMatrix());
        Renderer::frameConstants.camera.viewMatrix = Renderer::camera.getViewMatrix();
        Renderer::frameConstants.camera.cameraPos = glm::vec4(Renderer::camera.position, 1);
        Renderer::frameConstantsGPU.dataTransfer(&Renderer::frameConstants, sizeof(Renderer::FrameConstants));
        lightTransfer();
        //cullLights(); //light culling
        uint32_t zero = 0;
        Renderer::activeLights.dataTransfer(&zero, 4); //set number of lights in activeLightList to 0

        //rendering scene
        cmd.buffer.reset();
        cmd.buffer.begin(vk::CommandBufferBeginInfo());

        //compute light culling
        cmd.buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, Renderer::computeLayout, 0
                                      , sizeof(Renderer::descriptorSets) / sizeof(vk::DescriptorSet),
                                      Renderer::descriptorSets, 0, nullptr);
        vk::ShaderStageFlagBits stageCompute = vk::ShaderStageFlagBits::eCompute;
        cmd.buffer.bindShadersEXT(1, &stageCompute, &lightCullingShader.shader);
        cmd.buffer.dispatch(3, 1, 1);
        //barrier between compute and fragment shader
        vk::MemoryBarrier computeBarrier;
        cmd.buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                   vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags(),
                                   1, &computeBarrier, 0, nullptr, 0, nullptr);



        cmd.transitionImage(Renderer::swapchainImages[Renderer::imageIndex],
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eAttachmentOptimal,
                            vk::AccessFlagBits::eNone, vk::AccessFlagBits::eShaderWrite,
                            vk::PipelineStageFlagBits::eNone, vk::PipelineStageFlagBits::eFragmentShader,
                            vk::ImageAspectFlagBits::eColor
        );
        cmd.transitionImage(Renderer::depthBuffer.image,
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
                            vk::AccessFlagBits::eNone,
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                            vk::AccessFlagBits::eDepthStencilAttachmentRead,
                            vk::PipelineStageFlagBits::eNone, vk::PipelineStageFlagBits::eEarlyFragmentTests,
                            vk::ImageAspectFlagBits::eDepth);

        cmd.clearImage(Renderer::swapchainImageViews[Renderer::imageIndex],
                       vk::ClearColorValue(0.2f, 0.0f, 0.2f, 1.0f), Renderer::depthBuffer.view);

        cmd.beginRendering(Renderer::swapchainImageViews[Renderer::imageIndex], Renderer::depthBuffer.view);
        cmd.stupidBoilerPlate();
        cmd.enableDepth();


        vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eVertex;
        cmd.buffer.bindShadersEXT(1, &stage, &vertextShader.shader);
        stage = vk::ShaderStageFlagBits::eFragment;
        cmd.buffer.bindShadersEXT(1, &stage, &fragmentShader.shader);

        cmd.buffer.setVertexInputEXT(1, &Vertex::bindingDescription,
                                     sizeof(Vertex::attributeDescription) / sizeof(
                                         vk::VertexInputAttributeDescription2EXT),
                                     Vertex::attributeDescription);
        cmd.buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Renderer::pipelineLayout, 0, 3,
                                      Renderer::descriptorSets, 0, nullptr);

        world.each([&](Transform &transform, Mesh &mesh) {
            cmd.drawMesh(mesh, transform);
        });

        cmd.buffer.endRenderingKHR();
        cmd.buffer.end();


        vk::CommandBufferSubmitInfo cmdBufferInfo;
        cmdBufferInfo.commandBuffer = cmd.buffer;
        vk::SubmitInfo2 submitInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cmdBufferInfo;
        grQueue.submit2(1, &submitInfo, nullptr);
        grQueue.waitIdle();
        //drawing ui
        editorDrawing(Renderer::imguiCmd);
        proccessInput();


        Renderer::device.waitForFences(1, &Renderer::finishedRenderingFence, true,UINT64_MAX);

        //////////////
        ///PRESENT///
        ////////////
        vk::PresentInfoKHR presentInfo;
        presentInfo.swapchainCount = 1;
        presentInfo.pImageIndices = &Renderer::imageIndex;
        presentInfo.pSwapchains = &Renderer::swapchain;

        vk::Result presentResult = presentQueue.presentKHR(&presentInfo);
        if (presentResult != vk::Result::eSuccess) {
            Renderer::resizeSwapchain();
            continue;
        }
    }
}


void lightTransfer() {
    Renderer::lightData.numberOfLights = 0;
    world.each([](Transform &transform, PointLight &light) {
        Renderer::lightData.lights[Renderer::lightData.numberOfLights].intensity = light.intensity;
        Renderer::lightData.lights[Renderer::lightData.numberOfLights].color = light.color;
        Renderer::lightData.lights[Renderer::lightData.numberOfLights].position = transform.position;
        Renderer::lightData.numberOfLights += 1;
    });

    Renderer::lightBuffer.dataTransfer(&Renderer::lightData, sizeof(Renderer::LightData));
}

void cullLights() {
    uint32_t zero = 0;
    Renderer::activeLights.dataTransfer(&zero, sizeof(uint32_t));

    CmdBuffer cmd(vkb::QueueType::graphics);
    cmd.buffer.begin(vk::CommandBufferBeginInfo());
    cmd.buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, Renderer::computeLayout, 0
                                  , sizeof(Renderer::descriptorSets) / sizeof(vk::DescriptorSet),
                                  Renderer::descriptorSets, 0, nullptr);
    vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eCompute;
    cmd.buffer.bindShadersEXT(1, &stage, &lightCullingShader.shader);
    cmd.buffer.dispatch(3, 1, 1);
    cmd.buffer.end();

    vk::CommandBufferSubmitInfo cmdInfo;
    cmdInfo.commandBuffer = cmd.buffer;
    vk::SubmitInfo2 submit;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdInfo;
    grQueue.submit2(1, &submit, nullptr);
    grQueue.waitIdle();
}


void mainMenuBar();

void entityListPanel();

bool entityListVisible = true;

void basicInfo();

bool basicInfoVisible = true;

void entityInspector();

bool entityInspectorVisible = true;


void editorDrawing(CmdBuffer &cmd) {
    cmd.buffer.reset();
    cmd.buffer.begin(vk::CommandBufferBeginInfo());
    cmd.beginRendering(Renderer::swapchainImageViews[Renderer::imageIndex]);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, 0, ImGuiDockNodeFlags_PassthruCentralNode);

    mainMenuBar();

    basicInfo();
    entityListPanel();
    entityInspector();


    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(
        ImGui::GetDrawData(),
        cmd.buffer
    );
    cmd.buffer.endRenderingKHR();

    cmd.transitionImage(Renderer::swapchainImages[Renderer::imageIndex],
                        vk::ImageLayout::eAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
                        vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eNone,
                        vk::PipelineStageFlagBits2::eFragmentShader, vk::PipelineStageFlagBits2::eBottomOfPipe,
                        Renderer::vkbDevice.get_queue_index(vkb::QueueType::graphics).value(),
                        Renderer::vkbDevice.get_queue_index(vkb::QueueType::present).value(),
                        vk::ImageAspectFlagBits::eColor
    );

    cmd.buffer.end();

    vk::SubmitInfo2 submitInfo;
    submitInfo.commandBufferInfoCount = 1;
    vk::CommandBufferSubmitInfo commandBufferSubmitInfo;
    submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;
    commandBufferSubmitInfo.commandBuffer = cmd.buffer;
    grQueue.submit2(1, &submitInfo, Renderer::finishedRenderingFence);
    grQueue.waitIdle();
}

void mainMenuBar() {
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("basic info", 0, &basicInfoVisible);
        ImGui::MenuItem("entity list", 0, &entityListVisible);
        ImGui::MenuItem("entity inspector", 0, &entityInspectorVisible);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}


void basicInfo() {
    if (!basicInfoVisible) {
        return;
    }
    ImGui::Begin("basic info", &basicInfoVisible);

    ImGui::SeparatorText("CAMERA");
    ImGui::Text("\nX:%.2f Y:%.2f Z:%.2f\nY:%.2f P:%.2f",
                Renderer::camera.position.x, Renderer::camera.position.y, Renderer::camera.position.z,
                Renderer::camera.rotation.y, Renderer::camera.rotation.x);

    ImGui::End();
}

void entityListPanel() {
    static char newEntityName[32] = "\0";
    if (!entityListVisible) {
        return;
    }
    ImGui::Begin("entity panel", &entityListVisible);

    if (ImGui::Button("add entity")) {
        if (newEntityName[0] != '\0') {
            flecs::entity e = world.entity(newEntityName);
            e.add<Transform>();
            newEntityName[0] = '\0';
        }
    }
    ImGui::InputText("name", newEntityName, 32);

    world.each([](flecs::entity e, Transform &transform) {
        if (ImGui::TreeNode(e.name().c_str())) {
            if (ImGui::IsItemClicked()) {
                selectedEntity = e;
            }
            ImGui::TreePop();
        }
    });

    ImGui::End();
}

void entityInspector() {
    static char nothingSelected[] = "nothing";
    if (!entityInspectorVisible) {
        return;
    }
    ImGui::Begin("entity inspector");

    if (selectedEntity.id() == 0) {
        ImGui::SeparatorText("NO ENTITY SELECTED");
        ImGui::End();
        return;
    }

    ImGui::Text("NAME:%s", selectedEntity.name().c_str());
    ImGui::Text("ID:%d", selectedEntity.id());

    if (ImGui::CollapsingHeader("TRANSFORM")) {
        Transform &transform = selectedEntity.get_mut<Transform>();
        ImGui::DragFloat3("POSITION", (float *) &transform.position);
        ImGui::DragFloat4("ROTATION", (float *) &transform.rotation);
        transform.rotation = glm::normalize(transform.rotation);
        ImGui::DragFloat3("SCALE", (float *) &transform.scale);
        ImGui::Separator();
    }

    Mesh *mesh = selectedEntity.try_get_mut<Mesh>();
    if (mesh != nullptr) {
        if (ImGui::CollapsingHeader("MESH")) {
            char *geometryComboValue;
            if (mesh->geometryIndex == -1) {
                geometryComboValue = nothingSelected;
            } else {
                geometryComboValue = Renderer::meshGeometries[mesh->geometryIndex].name;
            }
            if (ImGui::BeginCombo("geometry", geometryComboValue)) {
                if (ImGui::Selectable(nothingSelected)) {
                    mesh->geometryIndex = -1;
                }
                for (int i = 0; i < 1024; i++) {
                    MeshGeometry &geometry = Renderer::meshGeometries[i];
                    if (geometry.name != nullptr) {
                        if (ImGui::Selectable(geometry.name)) {
                            mesh->geometryIndex = i;
                        }
                    }
                }
                ImGui::EndCombo();
            }

            char *materialComboValue;
            if (mesh->materialIndex == -1) {
                materialComboValue = nothingSelected;
            } else {
                materialComboValue = Renderer::materials[mesh->materialIndex].name;
            }
            if (ImGui::BeginCombo("material", materialComboValue)) {
                if (ImGui::Selectable(nothingSelected)) {
                    mesh->materialIndex = -1;
                }
                for (int i = 0; i < 1024; i++) {
                    Material &material = Renderer::materials[i];
                    if (material.name != nullptr) {
                        if (ImGui::Selectable(material.name)) {
                            mesh->materialIndex = i;
                        }
                    }
                }
                ImGui::EndCombo();
            }


            if (ImGui::Button("REMOVE##MESH")) {
                selectedEntity.remove<Mesh>();
            }
            ImGui::Separator();
        }
    }

    PointLight *pointLight = selectedEntity.try_get_mut<PointLight>();
    if (pointLight != nullptr) {
        if (ImGui::CollapsingHeader("LIGHT")) {
            ImGui::DragFloat("intensity", &pointLight->intensity, 0.1f);
            ImGui::ColorEdit3("lightColor", (float *) &pointLight->color, ImGuiColorEditFlags_Float);
            if (ImGui::Button("REMOVE##LIGHT")) {
                selectedEntity.remove<PointLight>();
            }
            ImGui::Separator();
        }
    }


    //Buttons for adding components
    if (pointLight == nullptr) {
        if (ImGui::Button("add light")) {
            selectedEntity.add<PointLight>();
            PointLight &l = selectedEntity.get_mut<PointLight>();
            l.color = {1, 1, 1};
            l.intensity = 5;
        }
    }
    if (mesh == nullptr) {
        if (ImGui::Button("add mesh")) {
            selectedEntity.add<Mesh>();
            Mesh &m = selectedEntity.get_mut<Mesh>();
            m.geometryIndex = -1;
            m.materialIndex = -1;
        }
    }


    ImGui::End();
}

void proccessInput() {
    if (Input::getCameraPause()) {
        return;
    }

    glm::vec4 move = {0, 0, 0, 1};
    if (Input::getForward()) {
        move.z += 1;
    }
    if (Input::getBackward()) {
        move.z -= 1;
    }
    if (Input::getLeft()) {
        move.x -= 1;
    }
    if (Input::getRight()) {
        move.x += 1;
    }

    move = glm::eulerAngleY(-glm::radians(Renderer::camera.rotation.y)) * move;

    if (Input::getUp()) {
        move.y += 1;
    }
    if (Input::getDown()) {
        move.y -= 1;
    }

    float speed = 0.3;
    float mouseSpeed = 2.5;
    move *= speed;
    Renderer::camera.position += glm::vec3(move.x, move.y, move.z);

    Renderer::camera.rotation.x -= Input::getMouseY() * mouseSpeed;
    Renderer::camera.rotation.y -= Input::getMouseX() * mouseSpeed;
}

