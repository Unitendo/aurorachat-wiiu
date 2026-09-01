#include "input.h"
#include "chat.h"
#include "net.h"
#include "storage.h"

int mainMenuIndex = 0;
int settingsMenuIndex = 0;
int selectionMenuIndex = 0;
int authMenuIndex = 0;

bool AutoLoginEnabled = true;

void handle_button_down(const SDL_ControllerButtonEvent& e)
{
    if (currentTextSendType == type_none) {
        if (scene == CHAT) {
            if (e.button == SDL_CONTROLLER_BUTTON_A) {
                if (connectionLost) {
                    ReconnectToTCPServer();
                }
                else {
                    currentTextSendType = type_message;
                    SDL_WiiUSetSWKBDHintText("Say something...");
                    SDL_StartTextInput();
                }
            }
            if (e.button == SDL_CONTROLLER_BUTTON_B) {
                scene = MAIN_MENU;
            }
        }
        else if (scene == RULES) {
            if (e.button == SDL_CONTROLLER_BUTTON_A) {
                scene = CHAT;
            }
        }
        else if (scene == SETTINGS) {
            if (e.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {

                settingsMenuIndex--;

                if (settingsMenuIndex < 0)
                    settingsMenuIndex = 1;
            }

            else if (e.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {

                settingsMenuIndex++;

                if (settingsMenuIndex > 1)
                    settingsMenuIndex = 0;
            }

            else if (e.button == SDL_CONTROLLER_BUTTON_A) {

                switch (settingsMenuIndex) {

                    case 0:
                        AutoLoginEnabled = !AutoLoginEnabled;
                        SaveSettings(AutoLoginEnabled);
                        break;

                    case 1:
                        scene = MAIN_MENU;
                        break;
                }
            }
        }
        else if (scene == CREDITS) {
            if (e.button == SDL_CONTROLLER_BUTTON_B)
                scene = MAIN_MENU;
        }
        else if (scene == MAIN_MENU) {
            if (e.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {

                mainMenuIndex--;

                if (mainMenuIndex < 0)
                    mainMenuIndex = 3;
            }

            else if (e.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {

                mainMenuIndex++;

                if (mainMenuIndex > 3)
                    mainMenuIndex = 0;
            }

            else if (e.button == SDL_CONTROLLER_BUTTON_A) {

                switch (mainMenuIndex) {

                    case 0:
                        scene = CHAT;
                        break;

                    case 1:
                        scene = RULES;
                        request_rules();
                        break;

                    case 2:
                        scene = SETTINGS;
                        break;
                    case 3:
                        scene = CREDITS;
                        break;
                }
            }
        }
        else if (scene == SELECTION_MENU) {
            if (e.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {

                selectionMenuIndex--;

                if (selectionMenuIndex < 0)
                    selectionMenuIndex = 1;
            }

            else if (e.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {

                selectionMenuIndex++;

                if (selectionMenuIndex > 1)
                    selectionMenuIndex = 0;
            }

            else if (e.button == SDL_CONTROLLER_BUTTON_A) {

                switch (selectionMenuIndex) {

                    case 0:
                        scene = SIGN_UP;
                        authMenuIndex = 0;
                        break;

                    case 1:
                        scene = SIGN_IN;
                        authMenuIndex = 0;
                        break;
                }
            }
        }
        else if (scene == SIGN_UP || scene == SIGN_IN) {
            if (e.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {

                authMenuIndex--;

                if (authMenuIndex < 0)
                    authMenuIndex = 2;
            }

            else if (e.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {

                authMenuIndex++;

                if (authMenuIndex > 2)
                    authMenuIndex = 0;
            }

            else if (e.button == SDL_CONTROLLER_BUTTON_A) {

                switch (authMenuIndex) {

                    case 0:
                        currentTextSendType = type_username;
                        SDL_WiiUSetSWKBDInitialText(username.c_str());
                        SDL_WiiUSetSWKBDHintText("Enter a username...");
                        SDL_StartTextInput();
                        authMenuIndex = 0;
                        break;

                    case 1:
                        currentTextSendType = type_password;
                        SDL_WiiUSetSWKBDInitialText(password.c_str());
                        SDL_WiiUSetSWKBDHintText("Enter a password...");
                        SDL_WiiUSetSWKBDPasswordMode(SDL_WIIU_SWKBD_PASSWORD_MODE_HIDE);
                        SDL_StartTextInput();
                        break;

                    case 2:
                    if (username.empty() || password.empty()) {
                        break;
                    }

                    if (scene == SIGN_UP)
                    {
                        bool ok = create_account(username.c_str(), password.c_str());

                        if (!ok)
                        {
                            scene = SELECTION_MENU;
                            break;
                        }

                        SaveLogin(username.c_str(), password.c_str());
                        join_room("general");
                        request_rules();
                        request_history("1024");

                        scene = RULES;
                    }
                    else if (scene == SIGN_IN)
                    {
                        bool ok = login_account(username.c_str(), password.c_str());

                        if (!ok)
                        {
                            scene = SELECTION_MENU;
                            break;
                        }

                        SaveLogin(username.c_str(), password.c_str());
                        join_room("general");
                        request_rules();
                        request_history("1024");

                        scene = RULES;
                    }

                    break;
                }
            }
            else if (e.button == SDL_CONTROLLER_BUTTON_B) {
                scene = SELECTION_MENU;
            }
        }
    }
}

void handle_event(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
            SDL_GameControllerOpen(event.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (auto ctrlr = SDL_GameControllerFromInstanceID(event.cdevice.which))
                SDL_GameControllerClose(ctrlr);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            handle_button_down(event.cbutton);
            break;
    }
}
