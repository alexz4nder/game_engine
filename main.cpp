#include <iostream>
#include <fmt/printf.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION
#include "external/VulkanMemoryAllocator-Hpp/include/vk_mem_alloc.hpp"

#include "external/vk-bootstrap/src/VkBootstrap.h"

#include "Rendering/Renderer.h"
#include <SDL3/SDL.h>
#include <thread>

#include "imgui_impl_sdl3.h"
#include "flecs.h"
#include "InputManagment/Input.h"
#include "util/logging.h"

flecs::world world;
void editorMainLoop();

void renderThread() {
    editorMainLoop();
}
std::ostream &operator<<(std::ostream &os, glm::vec3 v);
int main() {
    Renderer::initializeRenderer();


    std::thread rendererThread(renderThread);
    while (1) {
        SDL_Event ev;
        if (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type==SDL_EVENT_KEY_DOWN||ev.type==SDL_EVENT_KEY_UP||ev.type==SDL_EVENT_MOUSE_MOTION) {
                Input::proccessInput(ev);
            }
            if (ev.type==SDL_EVENT_QUIT) {
                fnLogg("quiting");
                return 0;
            }
            if (ev.type==SDL_EVENT_WINDOW_RESIZED) {
                //fmt::println("SDL RESIZE");
            }

        }


    }

    return 0;
}
