#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <string>

extern std::string username;
extern std::string password;

enum textSendType {
    type_username,
    type_password,
    type_message,
    type_none
};

extern textSendType currentTextSendType;

extern std::string currentRoom;

enum Scene {
    SELECTION_MENU,
    SIGN_UP,
    SIGN_IN,
    RULES,
    CHAT
};

extern Scene scene;

extern int selectionMenuIndex;
extern int authMenuIndex;

extern bool showResponse;

void handle_button_down(const SDL_ControllerButtonEvent& e);
void handle_event(const SDL_Event& event);
