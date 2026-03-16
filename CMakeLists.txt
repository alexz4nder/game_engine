cmake_minimum_required(VERSION 4.1)
project(game_engine)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/VulkanMemoryAllocator-Hpp)
add_subdirectory(external/vk-bootstrap)
add_subdirectory(external/fastgltf)
include_directories(external/imgui)
include_directories(external/flecs)

add_executable(game_engine main.cpp
        Rendering/RendererInit.cpp
        util/logging.h
        EditorMainLoop.cpp
        Rendering/Renderer.cpp
        external/imgui/imgui.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imgui_impl_vulkan.cpp
        external/imgui/imgui_impl_sdl3.cpp
        Rendering/CmdBuffer.cpp
        Rendering/CmdBuffer.h
        Rendering/BufferGPU.cpp
        Rendering/BufferGPU.h
        Rendering/ShaderObj.cpp
        Rendering/ShaderObj.h
        Rendering/Vertex.cpp
        Rendering/Vertex.h
        Rendering/MeshGeometry.cpp
        Rendering/MeshGeometry.h
        Rendering/Camera.cpp
        Rendering/Camera.h
        InputManagment/Input.cpp
        InputManagment/Input.h
        InputManagment/InputGetters.cpp
        external/flecs/flecs.c
        ecs_components/Transform.cpp
        ecs_components/Transform.h
        loadScene.cpp
        ecs_components/Mesh.cpp
        ecs_components/Mesh.h
        Rendering/ImageGPU.cpp
        Rendering/ImageGPU.h
        external/stb/stbi_implementation.cpp
        Rendering/Material.cpp
        Rendering/Material.h
        ecs_components/PointLight.cpp
        ecs_components/PointLight.h)

target_link_libraries(game_engine vulkan SDL3 vk-bootstrap::vk-bootstrap fmt VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp fastgltf::fastgltf)
