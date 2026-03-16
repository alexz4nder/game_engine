//
// Created by aco on 10/03/2026.
//

#include "Vertex.h"

void Vertex::initVertexAttribute() {
    bindingDescription.binding=0;
    bindingDescription.stride=sizeof(Vertex);
    bindingDescription.inputRate=vk::VertexInputRate::eVertex;
    bindingDescription.divisor=1;

    attributeDescription[0].binding=0;
    attributeDescription[0].format=vk::Format::eR32G32B32Sfloat;
    attributeDescription[0].location=0;
    attributeDescription[0].offset=offsetof(Vertex,position);

    attributeDescription[1].binding=0;
    attributeDescription[1].format=vk::Format::eR32G32B32Sfloat;
    attributeDescription[1].location=1;
    attributeDescription[1].offset=offsetof(Vertex,normal);

    attributeDescription[2].binding=0;
    attributeDescription[2].format=vk::Format::eR32G32B32Sfloat;
    attributeDescription[2].location=2;
    attributeDescription[2].offset=offsetof(Vertex,tangent);

    attributeDescription[3].binding=0;
    attributeDescription[3].format=vk::Format::eR32G32B32Sfloat;
    attributeDescription[3].location=3;
    attributeDescription[3].offset=offsetof(Vertex,biTangent);

    attributeDescription[4].binding=0;
    attributeDescription[4].format=vk::Format::eR32G32Sfloat;
    attributeDescription[4].location=4;
    attributeDescription[4].offset=offsetof(Vertex,texCoord);
}
