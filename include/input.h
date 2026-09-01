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
    type_room,
    type_none
};

extern textSendType currentTextSendType;

extern std::string currentRoom;

enum Scene {
    MAIN_MENU,
    SELECTION_MENU,
    SIGN_UP,
    SIGN_IN,
    RULES,
    SETTINGS,
    CREDITS,
    CHAT
};

extern Scene scene;

extern int mainMenuIndex;
extern int settingsMenuIndex;
extern int selectionMenuIndex;
extern int authMenuIndex;

extern bool AutoLoginEnabled;

void handle_button_down(const SDL_ControllerButtonEvent& e);
void handle_event(const SDL_Event& event);
