//
// Created by aco on 08/03/2026.
//


#ifndef GAME_ENGINE_LOGGING_H
#define GAME_ENGINE_LOGGING_H


#define fnLogg(...){\
fmt::print("[{}]",__func__);\
fmt::println(__VA_ARGS__);\
}\




#endif //GAME_ENGINE_LOGGING_H