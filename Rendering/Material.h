//
// Created by aco on 12/03/2026.
//

#ifndef GAME_ENGINE_MATERIAL_H
#define GAME_ENGINE_MATERIAL_H
#include <cinttypes>

struct Material {
    uint32_t colorIndex;
    uint32_t normalIndex;

    char *name;
};


#endif //GAME_ENGINE_MATERIAL_H