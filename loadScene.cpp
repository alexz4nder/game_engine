//
// Created by aco on 11/03/2026.
//


#include <filesystem>
#include "util/logging.h"
#include "fastgltf/core.hpp"
#include "flecs.h"
#include "ecs_components/Transform.h"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/tools.hpp"
#include "Rendering/Renderer.h"
#include "Rendering/Vertex.h"
#include "ecs_components/Mesh.h"
#include "ecs_components/PointLight.h"
#include "stb_image.h"

extern flecs::world world;

void copyTRS(fastgltf::TRS &trs,Transform &transform);
void copyMesh(fastgltf::Asset &asset,MeshGeometry &meshGeometry,fastgltf::Mesh &mesh);
void copyTexture(fastgltf::Asset &asset,ImageGPU &imageGPU,fastgltf::Texture &texture);
void updateTexture(ImageGPU &imageGPU,uint32_t index);
std::filesystem::path parentDir;


const float lightIntensityTransfer=3261.085/60;
void loadScene(std::filesystem::path path) {
    fnLogg("FILE NAME:{}",path.c_str());
    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(path);
    if (gltfFile.error()!=fastgltf::Error::None) {
        fnLogg("error opening file");
        abort();
    }

    parentDir=path.parent_path();
    fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);
    fastgltf::Options options= fastgltf::Options::LoadExternalBuffers;
    auto gltfAsset = parser.loadGltf(gltfFile.get(),path.parent_path(),options);
    if (gltfAsset.error()!=fastgltf::Error::None) {
        fnLogg("cant parse file");
        abort();
    }
    fastgltf::Asset &asset = gltfAsset.get();


    //adding meshes
    for (int i =0;i<asset.meshes.size();i++) {
        fastgltf::Mesh &mesh = asset.meshes.at(i);
        MeshGeometry &meshGeometry = Renderer::meshGeometries[i];
        meshGeometry.name=new char[mesh.name.size()];
        strcpy(meshGeometry.name,mesh.name.c_str());
        copyMesh(asset,meshGeometry,mesh);
    }
    //adding textures
    for (int i = 0; i < asset.textures.size(); ++i) {
        fastgltf::Texture &texture = asset.textures[i];
        ImageGPU &imageGPU =Renderer::textures[i];
        imageGPU.name=new char[texture.name.size()];
        strcpy(imageGPU.name,texture.name.c_str());
        copyTexture(asset,imageGPU,texture);
        updateTexture(imageGPU,i);
    }
    //adding materials
    for (int i =0;i<asset.materials.size();i++) {
        Material &material = Renderer::materials[i];
        fastgltf::Material &gltfMaterial = asset.materials[i];

        material.name=new char[gltfMaterial.name.size()];
        strcpy(material.name,gltfMaterial.name.c_str());

        if (gltfMaterial.pbrData.baseColorTexture.has_value()) {
            material.colorIndex=gltfMaterial.pbrData.baseColorTexture.value().textureIndex;
        }else{material.colorIndex=-1;}

        if (gltfMaterial.normalTexture.has_value()) {
            material.normalIndex=gltfMaterial.normalTexture.value().textureIndex;
        }else{material.normalIndex=-1;}
    }

    //addint nodes
    for (fastgltf::Node node:asset.nodes) {
        flecs::entity e = world.entity(node.name.c_str());
        e.add<Transform>();
        Transform &transform = e.get_mut<Transform>();
        fastgltf::TRS &trs =std::get<fastgltf::TRS>(node.transform);
        copyTRS(trs,transform);
        if (node.meshIndex.has_value()) {
            e.add<Mesh>();
            Mesh &mesh = e.get_mut<Mesh>();
            mesh.geometryIndex=node.meshIndex.value();
            if (asset.meshes[mesh.geometryIndex].primitives[0].materialIndex.has_value()) {
                mesh.materialIndex=asset.meshes[mesh.geometryIndex].primitives[0].materialIndex.value();
            }else{mesh.materialIndex=-1;}
        }
        if (node.lightIndex.has_value()) {
            fastgltf::Light &light = asset.lights[node.lightIndex.value()];
            e.add<PointLight>();
            PointLight &pointLight = e.get_mut<PointLight>();
            pointLight.intensity=(light.intensity/lightIntensityTransfer)/10;
            pointLight.color.r=light.color.x();
            pointLight.color.g=light.color.y();
            pointLight.color.b=light.color.z();

        }



    }

}


void copyTRS(fastgltf::TRS &trs, Transform &transform) {
    transform.position.x=trs.translation.x();
    transform.position.y=trs.translation.y();
    transform.position.z=-trs.translation.z();

    transform.scale.x=trs.scale.x();
    transform.scale.y=trs.scale.y();
    transform.scale.z=trs.scale.z();

    transform.rotation.x=trs.rotation.x();
    transform.rotation.y=trs.rotation.y();
    transform.rotation.z=trs.rotation.z();
    transform.rotation.w=trs.rotation.w();
}

void copyMesh(fastgltf::Asset &asset,MeshGeometry &meshGeometry,fastgltf::Mesh &mesh) {
    fastgltf::Primitive &primitives = mesh.primitives[0];

    fastgltf::Attribute *positionAttribute=primitives.findAttribute("POSITION");
    fastgltf::Attribute *normalAttribute=primitives.findAttribute("NORMAL");
    fastgltf::Attribute *tangentAttribute=primitives.findAttribute("TANGENT");
    fastgltf::Attribute *texcoordAttribute=primitives.findAttribute("TEXCOORD_0");

    uint32_t numberOfVertices = asset.accessors[positionAttribute->accessorIndex].count;
    meshGeometry.numberOfVertices=numberOfVertices;
    Vertex *vertexArr = new Vertex[numberOfVertices];

    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,asset.accessors[positionAttribute->accessorIndex],
        [&](glm::vec3 pos,std::size_t index) {
            vertexArr[index].position=pos;
            vertexArr[index].position.z*=-1;
        });
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,asset.accessors[normalAttribute->accessorIndex],
            [&](glm::vec3 norm,std::size_t index) {
                vertexArr[index].normal=norm;
                vertexArr[index].normal.z*=-1;
        });
    fastgltf::iterateAccessorWithIndex<glm::vec4>(asset,asset.accessors[tangentAttribute->accessorIndex],
            [&](glm::vec4 tan,std::size_t index) {
                vertexArr[index].tangent={tan.x,tan.y,tan.z};
                vertexArr[index].tangent.z*=-1;
                vertexArr[index].biTangent=tan.w*glm::cross(vertexArr[index].normal,vertexArr[index].tangent);
        });
    fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,asset.accessors[texcoordAttribute->accessorIndex],
            [&](glm::vec2 texc,std::size_t index) {
                vertexArr[index].texCoord=texc;
        });

    meshGeometry.vertexBuffer.init(sizeof(Vertex)*numberOfVertices,
        vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eVertexBuffer);
    meshGeometry.vertexBuffer.dataTransfer(vertexArr,sizeof(Vertex)*numberOfVertices);
    delete[] vertexArr;


    fastgltf::Accessor &indicesAccessor = asset.accessors[primitives.indicesAccessor.value()];
    uint32_t numberOfIndices = indicesAccessor.count;
    meshGeometry.numberOfIndices=numberOfIndices;
    uint32_t *indexArr = new uint32_t[numberOfIndices];
    fastgltf::iterateAccessorWithIndex<uint32_t>(asset,indicesAccessor,
            [&](uint32_t num,std::size_t index) {
                indexArr[index]=num;
        });
    meshGeometry.indexBuffer.init(sizeof(uint32_t)*numberOfIndices,
        vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eIndexBuffer);
    meshGeometry.indexBuffer.dataTransfer(indexArr,sizeof(uint32_t)*numberOfIndices);
    delete[] indexArr;

    fnLogg("{} numberOfVertices:{} numberOfIndices:{}",mesh.name.c_str(),numberOfVertices,numberOfIndices);
}


void copyTexture(fastgltf::Asset &asset, ImageGPU &imageGPU, fastgltf::Texture &texture) {
    fastgltf::Image &image = asset.images[texture.imageIndex.value()];
    uint32_t sourceIndex = image.data.index();

    switch (sourceIndex) {
        case 2: {
            fastgltf::sources::URI &uri = std::get<fastgltf::sources::URI>(image.data);
            std::filesystem::path imagePath = parentDir;
            imagePath.append(uri.uri.c_str());
            fnLogg("{}",imagePath.c_str());
            FILE *f=fopen(imagePath.c_str(),"r");
            int x,y,channels;
            stbi_uc *imageData = stbi_load_from_file(f,&x,&y,&channels,STBI_rgb_alpha);
            fclose(f);

            imageGPU.init(vk::Extent3D(x,y,1),
                vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eSampled,
                vk::Format::eR8G8B8A8Srgb);
            imageGPU.transferImage(imageData, vk::ImageLayout::eShaderReadOnlyOptimal);
            imageGPU.createView();
            stbi_image_free(imageData);
        }break;
        default:
            fnLogg("SOURCE NOT SUPPORTED");
            abort();
    }

}

void updateTexture(ImageGPU &imageGPU, uint32_t index) {
    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageLayout=vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView=imageGPU.view;


    vk::WriteDescriptorSet descWrite;
    descWrite.descriptorCount=1;
    descWrite.dstArrayElement=index;
    descWrite.descriptorType=vk::DescriptorType::eSampledImage;
    descWrite.dstBinding=0;
    descWrite.dstSet=Renderer::descriptorSets[2];
    descWrite.pImageInfo=&imageInfo;

    Renderer::device.updateDescriptorSets(1,&descWrite,0,nullptr);
}
