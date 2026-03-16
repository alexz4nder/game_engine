//
// Created by aco on 10/03/2026.
//

#ifndef GAME_ENGINE_MESHGEOMETRY_H
#define GAME_ENGINE_MESHGEOMETRY_H
#include "BufferGPU.h"
#include <filesystem>
#include <glm/ext/scalar_uint_sized.hpp>

void loadScene(std::filesystem::path path);
namespace fastgltf {
    class Asset;
    class Mesh;
}

struct MeshGeometry {
    BufferGPU vertexBuffer;
    BufferGPU indexBuffer;
    uint32_t numberOfVertices;
    uint32_t numberOfIndices;
    char *name=nullptr;

    friend class CmdBuffer;
    friend void loadScene(std::filesystem::path);
    friend void copyMesh(fastgltf::Asset &asset,MeshGeometry &meshGeometry,fastgltf::Mesh &mesh);
};


#endif //GAME_ENGINE_MESHGEOMETRY_H