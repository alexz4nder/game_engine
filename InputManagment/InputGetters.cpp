//
// Created by aco on 10/03/2026.
//

#include "Input.h"

float Input::getMouseX(bool reset) {
    if (reset) {
        float t=mouseX;
        mouseX=0;
        return t;
    }
    return mouseX;
}
float Input::getMouseY(bool reset) {
    if (reset) {
        float t=mouseY;
        mouseY=0;
        return t;
    }
    return mouseY;
}

bool Input::getBackward() {
    return backward;
}

bool Input::getForward() {
    return forward;
}

bool Input::getLeft() {
    return left;
}

bool Input::getRight() {
    return right;
}

bool Input::getUp() {
    return up;
}

bool Input::getDown() {
    return down;
}

bool Input::getCameraPause() {
    return pause;
}
