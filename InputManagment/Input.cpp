//
// Created by aco on 10/03/2026.
//

#include "Input.h"

#include "../util/logging.h"

void Input::proccessInput(SDL_Event &ev) {
    //fnLogg("input");
    if (ev.type==SDL_EVENT_MOUSE_MOTION) {
        mouseX=ev.motion.xrel;
        mouseY=ev.motion.yrel;
        return;
    }
    if (ev.type==SDL_EVENT_KEY_DOWN) {
        switch (ev.key.scancode) {
            case SDL_SCANCODE_A:
                left=true;
                break;
            case SDL_SCANCODE_W:
                forward=true;
                break;
            case SDL_SCANCODE_S:
                backward=true;
                break;
            case SDL_SCANCODE_D:
                right=true;
                break;
            case SDL_SCANCODE_LSHIFT:
                up=true;
                break;
            case SDL_SCANCODE_LCTRL:
                down=true;
                break;
            case SDL_SCANCODE_P:
                pause=!pause;
                break;
        }
        return;
    }
    else if (ev.type==SDL_EVENT_KEY_UP) {
        switch (ev.key.scancode) {
            case SDL_SCANCODE_A:
                left=false;
                break;
            case SDL_SCANCODE_W:
                forward=false;
                break;
            case SDL_SCANCODE_S:
                backward=false;
                break;
            case SDL_SCANCODE_D:
                right=false;
                break;
            case SDL_SCANCODE_LSHIFT:
                up=false;
                break;
            case SDL_SCANCODE_LCTRL:
                down=false;
                break;

        }
        return;
    }

}
