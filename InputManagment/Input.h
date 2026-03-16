//
// Created by aco on 10/03/2026.
//

#ifndef GAME_ENGINE_INPUT_H
#define GAME_ENGINE_INPUT_H
#include <SDL3/SDL.h>


class Input {
public:
    static void proccessInput(SDL_Event &ev);

private:
    inline static bool forward=false;
    inline static bool backward=false;
    inline static bool left=false;
    inline static bool right=false;
    inline static bool up=false;
    inline static bool down=false;
    inline static bool pause=false;
public:
    static bool getForward();
    static bool getBackward();
    static bool getLeft();
    static bool getRight();
    static bool getUp();
    static bool getDown();
    static bool getCameraPause();



private:
    inline static float mouseX;
    inline static float mouseY;
public:
    static float getMouseX(bool reset=true);
    static float getMouseY(bool reset=true);
};


#endif //GAME_ENGINE_INPUT_H