#include <whb/proc.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <romfs-wiiu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>

#include "font.h"
#include "chat.h"
#include "image.h"
#include "net.h"
#include "input.h"
#include "storage.h"
#include "scale.h"


std::string username = "";
std::string password = "";

textSendType currentTextSendType = type_none;

Scene scene = SELECTION_MENU;

int fontSize = 48;
int maxWidth = 0;

int sock = ConnectToTCPServer();

bool connectionLost = false;

SDL_Window *tvWindow = NULL;
SDL_Window *drcWindow = NULL;
SDL_Renderer *tvRenderer = NULL;
SDL_Renderer *drcRenderer = NULL;

// TV colors
SDL_Color tvBackgroundColor = {0, 0, 0, 255};
SDL_Color tvTextColor = {255, 255, 255, 255};

// DRC colors
SDL_Color drcBackgroundColor = {0, 0, 0, 255};
SDL_Color drcTextColor = {255, 255, 255, 255};

static std::vector<std::string> WrapRulesText(const std::string& text, int fontSizePx, int wrapWidthPx)
{
    std::vector<std::string> outLines;

    int approxCharWidth = fontSizePx / 2;
    if (approxCharWidth < 1) approxCharWidth = 1;
    size_t maxCharsPerLine = (size_t)(wrapWidthPx / approxCharWidth);
    if (maxCharsPerLine < 10) maxCharsPerLine = 10;

    size_t start = 0;
    while (start <= text.size())
    {
        size_t nl = text.find('\n', start);
        std::string rawLine = (nl == std::string::npos)
            ? text.substr(start)
            : text.substr(start, nl - start);

        // word-wrap
        size_t pos = 0;
        while (pos < rawLine.size())
        {
            size_t remaining = rawLine.size() - pos;
            size_t take = std::min(maxCharsPerLine, remaining);

            if (take < remaining)
            {
                size_t lastSpace = rawLine.rfind(' ', pos + take);
                if (lastSpace != std::string::npos && lastSpace > pos)
                    take = lastSpace - pos;
            }

            outLines.push_back(rawLine.substr(pos, take));
            pos += take;
            while (pos < rawLine.size() && rawLine[pos] == ' ') pos++;
        }

        if (rawLine.empty())
            outLines.push_back("");

        if (nl == std::string::npos) break;
        start = nl + 1;
    }

    return outLines;
}


int main(int argc, char **argv)
{
    WHBProcInit();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);
    romfsInit();
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);

    mkdir("fs:/vol/external01/wiiu/apps/aurorachatforWiiU", 0777);

    // Keyboard Text Input Buffer
    std::string textBuffer = "";

    char input[512] = "";

    // Initialize audio to stop loading screen music from playing
    SDL_AudioSpec want{}, have{};
    want.freq = 48000;
    want.format = AUDIO_S16;
    want.channels = 2;
    want.samples = 4096;
    want.callback = nullptr;

    SDL_OpenAudio(&want, &have);
    SDL_PauseAudio(0);

    // Set vsync hint before creating windows
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // TV Window (primary display)
    tvWindow = SDL_CreateWindow("TV", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1280, 720,  // Window resolution (will be scaled to fit TV)
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_WIIU_TV_ONLY);
    if (tvWindow) {
        tvRenderer = SDL_CreateRenderer(tvWindow, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }

    int tvWidth = 0;
    int tvHeight = 0;

    SDL_GetRendererOutputSize(tvRenderer, &tvWidth, &tvHeight);

    scaleX = tvWidth / 1920.0f;
    scaleY = tvHeight / 1080.0f;

    maxWidth = tvWidth - SX(40);
    int chatViewHeight = tvHeight - SY(80);

    // GamePad Window
    drcWindow = SDL_CreateWindow("DRC",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        854, 480,  // Native GamePad resolution
        SDL_WINDOW_WIIU_GAMEPAD_ONLY | SDL_WINDOW_WIIU_PREVENT_SWAP);
    if (drcWindow) {
        drcRenderer = SDL_CreateRenderer(drcWindow, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    bool savedAutoLogin;
    bool savedAutoScroll;
    if (LoadSettings(savedAutoLogin, savedAutoScroll)) {
        AutoLoginEnabled = savedAutoLogin;
        AutoScrollEnabled = savedAutoScroll;
    }

    if (AutoLoginEnabled && LoadLogin(username, password) && !username.empty() && !password.empty()) {
        if (login_account(username.c_str(), password.c_str())) {
            join_room("general");
            request_history("1024");
            scene = MAIN_MENU;
        } else {
            scene = SELECTION_MENU;
        }
    } else {
        scene = SELECTION_MENU;
    }

    // Background texture
    SDL_Texture* bgTexture = LoadImage(tvRenderer, "romfs:/res/bg.png");
    SDL_Texture* bgTextureDRC = LoadImage(drcRenderer, "romfs:/res/bg.png");

    LoadAvatars();

    SDL_Texture* systemAvatar = LoadImage(tvRenderer, "romfs:/res/system.png");
    AddChatLine(tvRenderer, "System", "Welcome!", systemAvatar, SF(fontSize), SF(fontSize), tvTextColor, tvTextColor, maxWidth, chatViewHeight);

    Uint32 lastTicks = 0;
    const int AXIS_DEADZONE = 8000;  // deadzone for joystick
    const float MAX_SPEED = 1000.0f;  // pixels per second when stick is fully pushed

    SDL_Event event;
    SDL_GameController* gController = nullptr;

    if (SDL_NumJoysticks() > 0) {
        if (SDL_IsGameController(0)) {
            gController = SDL_GameControllerOpen(0);
        }
    }

    int Keyboard_Event;
    SDL_WiiUSysWMEventType Keyboard_Ok = SDL_WIIU_SYSWM_SWKBD_OK_FINISH_EVENT;
    SDL_WiiUSysWMEventType Keyboard_Cancel = SDL_WIIU_SYSWM_SWKBD_CANCEL_EVENT;

    const char* mainMenu[] = {
        "Chat",
        "Rules",
        "Settings",
        "Credits"
    };

    const char* settingsMenu[] = {
        "Auto-Login",
        "Auto-Scroll",
        "Log out",
        "Back to Main Menu"
    };

    const char* selectionMenu[] = {
        "Create Account",
        "Log In"
    };

    const char* signUpMenu[] = {
        "Enter a username",
        "Enter a password",
        "Create account"
    };

    const char* signInMenu[] = {
        "Enter your username",
        "Enter your password",
        "Log In"
    };

    lastTicks = SDL_GetTicks();
    while (WHBProcIsRunning()) {
        if (scene == CHAT || scene == RULES) {
            Uint32 now = SDL_GetTicks();
            float deltaSec = (now - lastTicks) / 1000.0f;
            lastTicks = now;

            if (gController) {
                // ANALOG STICK
                Sint16 axisLeftY = SDL_GameControllerGetAxis(gController, SDL_CONTROLLER_AXIS_LEFTY);
                Sint16 axisRightY = SDL_GameControllerGetAxis(gController, SDL_CONTROLLER_AXIS_RIGHTY);
                int* targetPosY = (scene == CHAT) ? &chatPosY : &rulesScrollY;

                if (axisLeftY > AXIS_DEADZONE || axisLeftY < -AXIS_DEADZONE ||
                    axisRightY > AXIS_DEADZONE || axisRightY < -AXIS_DEADZONE) {
                    float norm = axisLeftY / 32767.0f + axisRightY / 32767.0f;
                    float move = norm * MAX_SPEED * deltaSec;
                    *targetPosY -= (int)move;
                }

                // D-PAD
                if (SDL_GameControllerGetButton(gController, SDL_CONTROLLER_BUTTON_DPAD_UP)) {
                    *targetPosY += (int)(MAX_SPEED * deltaSec);
                }

                if (SDL_GameControllerGetButton(gController, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
                    *targetPosY -= (int)(MAX_SPEED * deltaSec);
                }
            }
        }

        while (SDL_PollEvent(&event)) {
            handle_event(event);

            if (event.type == SDL_TEXTINPUT)
                textBuffer += event.text.text;

            if (event.type == SDL_SYSWMEVENT) {
                Keyboard_Event = event.syswm.msg->msg.wiiu.event;
                if (Keyboard_Event == Keyboard_Ok || Keyboard_Event == Keyboard_Cancel) {
                    if (Keyboard_Event == Keyboard_Ok) {
                        if (currentTextSendType == type_message && !textBuffer.empty()) {
                            strncpy(input, textBuffer.c_str(), sizeof(input) - 1);
                            input[sizeof(input) - 1] = '\0';
                            send_chat(input);
                        }
                        else if (currentTextSendType == type_username) {
                            username = textBuffer;
                        }
                        else if (currentTextSendType == type_password) {
                            password = textBuffer;
                        }
                        else if (currentTextSendType == type_room && !textBuffer.empty()) {
                            join_room(textBuffer.c_str());
                            currentRoom = textBuffer;
                            chatLines.clear();
                            chatPosY = 0;
                            AddChatLine(tvRenderer, "System", ("Room changed to " + textBuffer).c_str(), systemAvatar, SF(fontSize), SF(fontSize), tvTextColor, tvTextColor, maxWidth, chatViewHeight);
                            request_history("1024");
                        }
                    }
                    textBuffer.clear();
                    currentTextSendType = type_none;
                    SDL_StopTextInput();
                }
            }
        }

        // Handle incoming messages
        TryReceive(&sock, tvRenderer, SF(fontSize), tvTextColor, maxWidth, chatViewHeight);

        // Render TV Screen
        if (tvRenderer) {
            SDL_RenderClear(tvRenderer);

            // Draw background image
            if (bgTexture) {
                SDL_RenderCopy(tvRenderer, bgTexture, NULL, NULL);
            } else {
                SDL_SetRenderDrawColor(tvRenderer,
                    tvBackgroundColor.r,
                    tvBackgroundColor.g,
                    tvBackgroundColor.b,
                    tvBackgroundColor.a);
                SDL_RenderClear(tvRenderer);
            }

            if (scene == SIGN_UP || scene == SIGN_IN) {
                const int authMenuCount = 3;
                        
                for (int i = 0; i < authMenuCount; i++) {
                    // Highlight selected item
                    if (authMenuIndex == i) {
                    
                        SDL_Rect highlightRect = {
                            0,
                            SY(180 + (60 * i)),
                            tvWidth,
                            SY(56)
                        };
                    
                        SDL_SetRenderDrawBlendMode(
                            tvRenderer,
                            SDL_BLENDMODE_BLEND
                        );
                    
                        SDL_SetRenderDrawColor(
                            tvRenderer,
                            0, 0, 0, 180
                        );
                    
                        SDL_RenderFillRect(
                            tvRenderer,
                            &highlightRect
                        );
                    }

                    if (scene == SIGN_UP) {
                        DrawText(
                            tvRenderer,
                            signUpMenu[i],
                            SX(40),
                            SY(180 + (60 * i)),
                            SF(48),
                            drcTextColor
                        );
                    }
                    else {
                        DrawText(
                            tvRenderer,
                            signInMenu[i],
                            SX(40),
                            SY(180 + (60 * i)),
                            SF(48),
                            drcTextColor
                        );
                    }
                }
            }

            if (scene == MAIN_MENU) {
                DrawText(tvRenderer, "AuroraChat for Wii U", SX(300), SY(20), SF(128), tvTextColor);

                const int mainMenuCount = 4;

                for (int i = 0; i < mainMenuCount; i++) {
                    // Highlight selected item
                    if (mainMenuIndex == i) {
                    
                        SDL_Rect highlightRect = {0, SY(180 + (60 * i)), tvWidth, SY(56)};
                    
                        SDL_SetRenderDrawBlendMode(tvRenderer, SDL_BLENDMODE_BLEND);
                    
                        SDL_SetRenderDrawColor(tvRenderer, 0, 0, 0, 180);
                    
                        SDL_RenderFillRect(tvRenderer, &highlightRect);
                    }

                    DrawText(tvRenderer, mainMenu[i], SX(40), SY(180 + (60 * i)), SF(48), tvTextColor);
                }
            }
            else if (scene == SETTINGS) {
                DrawText(tvRenderer, "Settings", SX(650), SY(20), SF(128), tvTextColor);

                const int settingsMenuCount = 4;

                for (int i = 0; i < settingsMenuCount; i++) {
                    // Highlight selected item
                    if (settingsMenuIndex == i) {
                    
                        SDL_Rect highlightRect = {0, SY(180 + (60 * i)), tvWidth, SY(56)};
                    
                        SDL_SetRenderDrawBlendMode(tvRenderer, SDL_BLENDMODE_BLEND);
                    
                        SDL_SetRenderDrawColor(tvRenderer, 0, 0, 0, 180);
                    
                        SDL_RenderFillRect(tvRenderer, &highlightRect);
                    }

                    DrawText(tvRenderer, settingsMenu[i], SX(40), SY(180 + (60 * i)), SF(48), tvTextColor);

                    if (i == 0) {
                        if (AutoLoginEnabled)
                            DrawText(tvRenderer, "ON", SX(600), SY(180 + (60 * i)), SF(48), {0, 255, 0, 255});
                        else
                            DrawText(tvRenderer, "OFF", SX(600), SY(180 + (60 * i)), SF(48), {255, 0, 0, 255});
                    }
                    else if (i == 1) {
                        if (AutoScrollEnabled)
                            DrawText(tvRenderer, "ON", SX(600), SY(180 + (60 * i)), SF(48), {0, 255, 0, 255});
                        else
                            DrawText(tvRenderer, "OFF", SX(600), SY(180 + (60 * i)), SF(48), {255, 0, 0, 255});
                    }
                }
            }
            else if (scene == CREDITS) {
                DrawText(tvRenderer, "AuroraChat for Wii U v7.1", SX(20), SY(20), SF(64), tvTextColor);
                DrawText(tvRenderer, "Client Developed by Funtum", SX(20), SY(180), SF(64), tvTextColor);
                DrawText(tvRenderer, "Server Developed by KwTheDsGuy and 3pm", SX(20), SY(260), SF(64), tvTextColor);
                DrawText(tvRenderer, "Icon and banner by hugh", SX(20), SY(340), SF(64), tvTextColor);

                DrawText(tvRenderer, "Press Ⓑ to go back", SX(20), SY(1000), SF(64), tvTextColor);
            }
            else if (scene == SELECTION_MENU) {
                DrawText(tvRenderer, "Account Setup", SX(450), SY(50), SF(128), tvTextColor);

                if (showResponse) {
                    DrawText(tvRenderer, ("Error: " + authError).c_str(), SX(20), SY(860), SF(64), tvTextColor);
                }

                DrawText(tvRenderer, "Move: ↑/↓", SX(20), SY(930), SF(64), tvTextColor);
                DrawText(tvRenderer, "Select: Ⓐ", SX(20), SY(1000), SF(64), tvTextColor);

                const int selectionMenuCount = 2;
                        
                for (int i = 0; i < selectionMenuCount; i++) {
                    // Highlight selected item
                    if (selectionMenuIndex == i) {
                    
                        SDL_Rect highlightRect = {
                            0,
                            SY(180 + (60 * i)),
                            tvWidth,
                            SY(56)
                        };
                    
                        SDL_SetRenderDrawBlendMode(
                            tvRenderer,
                            SDL_BLENDMODE_BLEND
                        );
                    
                        SDL_SetRenderDrawColor(
                            tvRenderer,
                            0, 0, 0, 180
                        );
                    
                        SDL_RenderFillRect(
                            tvRenderer,
                            &highlightRect
                        );
                    }
                
                    DrawText(
                        tvRenderer,
                        selectionMenu[i],
                        SX(40),
                        SY(180 + (60 * i)),
                        SF(48),
                        tvTextColor
                    );
                }
            }
            else if (scene == SIGN_UP) {
                DrawText(tvRenderer, "Create Account", SX(450), SY(50), SF(128), tvTextColor);

                DrawText(tvRenderer, "Move: ↑/↓", SX(20), SY(930), SF(64), tvTextColor);
                DrawText(tvRenderer, "Select: Ⓐ", SX(20), SY(1000), SF(64), tvTextColor);
            }
            else if (scene == SIGN_IN) {
                DrawText(tvRenderer, "Logging In", SX(550), SY(50), SF(128), tvTextColor);

                DrawText(tvRenderer, "Move: ↑/↓", SX(20), SY(930), SF(64), tvTextColor);
                DrawText(tvRenderer, "Select: Ⓐ", SX(20), SY(1000), SF(64), tvTextColor);
            }
            else if (scene == RULES) {
                DrawText(tvRenderer, "Server Rules", SX(600), SY(50), SF(96), tvTextColor);

                if (!rulesLoaded) {
                    DrawText(tvRenderer, "Loading rules...", SX(450), SY(200), SF(128), tvTextColor);
                } else {
                    int lineFontSize = SF(36);
                    int lineHeight = lineFontSize + SY(8);
                    int wrapWidth = tvWidth - SX(80);
                
                    std::vector<std::string> lines = WrapRulesText(
                        serverRules.empty() ? "No rules provided." : serverRules,
                        lineFontSize,
                        wrapWidth
                    );

                    int viewTop = SY(180);
                    int viewBottom = tvHeight - SY(160);
                    int viewHeight = viewBottom - viewTop;

                    int contentHeight = (int)lines.size() * lineHeight;
                    int maxScroll = std::max(0, contentHeight - viewHeight);

                    if (rulesScrollY > 0) rulesScrollY = 0;
                    if (rulesScrollY < -maxScroll) rulesScrollY = -maxScroll;

                    SDL_Rect clipRect = { 0, viewTop, tvWidth, viewHeight };
                    SDL_RenderSetClipRect(tvRenderer, &clipRect);

                    int y = viewTop + rulesScrollY;
                    for (auto& line : lines) {
                        if (y + lineHeight >= viewTop && y <= viewBottom) {
                            DrawText(tvRenderer, line.c_str(), SX(40), y, lineFontSize, tvTextColor);
                        }
                        y += lineHeight;
                    }

                    SDL_RenderSetClipRect(tvRenderer, nullptr);
                }

                DrawText(tvRenderer, "Move: ↑/↓", SX(20), SY(930), SF(64), tvTextColor);
                DrawText(tvRenderer, "Accept & Continue: Ⓐ", SX(20), SY(1000), SF(64), tvTextColor);
            }
            else if (scene == CHAT) {
                DrawChatBuffer(tvRenderer, SX(40), SY(40));
            }
            SDL_RenderPresent(tvRenderer);
        }

        // Render DRC (GamePad) Screen
        if (drcRenderer) {
            SDL_RenderClear(drcRenderer);

            // Draw background image
            if (bgTextureDRC) {
                SDL_RenderCopy(drcRenderer, bgTextureDRC, NULL, NULL);
            } else {
                SDL_SetRenderDrawColor(drcRenderer,
                    drcBackgroundColor.r,
                    drcBackgroundColor.g,
                    drcBackgroundColor.b,
                    drcBackgroundColor.a);
                SDL_RenderClear(drcRenderer);
            }

            if (scene == SELECTION_MENU) {
                const int selectionMenuCount = 2;
                        
                for (int i = 0; i < selectionMenuCount; i++) {
                    // Highlight selected item
                    if (selectionMenuIndex == i) {
                    
                        SDL_Rect highlightRect = {
                            0,
                            40 * i,
                            854,
                            40
                        };
                    
                        SDL_SetRenderDrawBlendMode(
                            drcRenderer,
                            SDL_BLENDMODE_BLEND
                        );
                    
                        SDL_SetRenderDrawColor(
                            drcRenderer,
                            0, 0, 0, 180
                        );
                    
                        SDL_RenderFillRect(
                            drcRenderer,
                            &highlightRect
                        );
                    }
                
                    DrawText(
                        drcRenderer,
                        selectionMenu[i],
                        20,
                        40 * i + 4,
                        32,
                        drcTextColor
                    );
                }
            }
            else if (scene == SIGN_UP || scene == SIGN_IN) {
                const int authMenuCount = 3;
                        
                for (int i = 0; i < authMenuCount; i++) {
                    // Highlight selected item
                    if (authMenuIndex == i) {
                    
                        SDL_Rect highlightRect = {
                            0,
                            40 * i,
                            854,
                            40
                        };
                    
                        SDL_SetRenderDrawBlendMode(
                            drcRenderer,
                            SDL_BLENDMODE_BLEND
                        );
                    
                        SDL_SetRenderDrawColor(
                            drcRenderer,
                            0, 0, 0, 180
                        );
                    
                        SDL_RenderFillRect(
                            drcRenderer,
                            &highlightRect
                        );
                    }

                    if (scene == SIGN_UP) {
                        DrawText(
                            drcRenderer,
                            signUpMenu[i],
                            20,
                            40 * i + 4,
                            32,
                            drcTextColor
                        );
                    }
                    else {
                        DrawText(
                            drcRenderer,
                            signInMenu[i],
                            20,
                            40 * i + 4,
                            32,
                            drcTextColor
                        );
                    }
                }
            }
            else if (scene == MAIN_MENU) {
                const int mainMenuCount = 4;
                        
                for (int i = 0; i < mainMenuCount; i++) {
                    // Highlight selected item
                    if (mainMenuIndex == i) {
                    
                        SDL_Rect highlightRect = {0, 40 * i, 854, 40};
                    
                        SDL_SetRenderDrawBlendMode(drcRenderer, SDL_BLENDMODE_BLEND);
                    
                        SDL_SetRenderDrawColor(drcRenderer, 0, 0, 0, 180);
                    
                        SDL_RenderFillRect(drcRenderer, &highlightRect);
                    }
                
                    DrawText(drcRenderer, mainMenu[i], 20, 40 * i + 4, 32, drcTextColor);
                }
            }
            else if (scene == SETTINGS) {
                const int settingsMenuCount = 4;
                        
                for (int i = 0; i < settingsMenuCount; i++) {
                    // Highlight selected item
                    if (settingsMenuIndex == i) {
                    
                        SDL_Rect highlightRect = {0, 40 * i, 854, 40};
                    
                        SDL_SetRenderDrawBlendMode(drcRenderer, SDL_BLENDMODE_BLEND);
                    
                        SDL_SetRenderDrawColor(drcRenderer, 0, 0, 0, 180);
                    
                        SDL_RenderFillRect(drcRenderer, &highlightRect);
                    }
                
                    DrawText(drcRenderer, settingsMenu[i], 20, 40 * i, 36, drcTextColor);

                    if (i == 0) {
                        if (AutoLoginEnabled)
                            DrawText(drcRenderer, "ON", 300, 40 * i, 36, {0, 255, 0, 255});
                        else
                            DrawText(drcRenderer, "OFF", 300, 40 * i, 36, {255, 0, 0, 255});
                    }
                    else if (i == 1) {
                        if (AutoScrollEnabled)
                            DrawText(drcRenderer, "ON", 300, 40 * i, 36, {0, 255, 0, 255});
                        else
                            DrawText(drcRenderer, "OFF", 300, 40 * i, 36, {255, 0, 0, 255});
                    }
                }
            }
            else if (scene == CREDITS) {
                DrawText(drcRenderer, "AuroraChat for Wii U v7.1", 10, 10, 32, drcTextColor);
                DrawText(drcRenderer, "Client Developed by Funtum", 10, 90, 32, drcTextColor);
                DrawText(drcRenderer, "Server Developed by KwTheDsGuy and 3pm", 10, 130, 32, drcTextColor);
                DrawText(drcRenderer, "Icon and banner by hugh", 10, 170, 32, drcTextColor);

                DrawText(drcRenderer, "Press Ⓑ to go back", 10, 440, 32, drcTextColor);
            }
            else if (scene == SIGN_UP || scene == SIGN_IN) {
                DrawText(drcRenderer, "Enter text using the on-screen keyboard.", 20, 20, 32, drcTextColor);
            }
            else if (scene == RULES) {
                DrawText(drcRenderer, "Read the rules on TV", 10, 20, 48, drcTextColor);
                DrawText(drcRenderer, "Press Ⓐ to continue", 10, 440, 32, drcTextColor);
            }
            else if (scene == CHAT) {
                DrawText(drcRenderer, ("Username: " + username).c_str(), 10, 10, 48, drcTextColor);
                DrawText(drcRenderer, ("Room: " + currentRoom).c_str(), 10, 60, 48, drcTextColor);

                DrawText(drcRenderer, "Move: ↑/↓", SX(10), SY(320), SF(32), drcTextColor);
                DrawText(drcRenderer, "Leave: Ⓑ", SX(10), SY(360), SF(32), drcTextColor);
                DrawText(drcRenderer, "Change room: Ⓨ", SX(10), SY(400), SF(32), drcTextColor);
                DrawText(drcRenderer, "Send a message: Ⓐ", SX(10), SY(440), SF(32), drcTextColor);
            }
            SDL_RenderPresent(drcRenderer);
        }
    }

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }

    if (gController)
        SDL_GameControllerClose(gController);

    if (bgTexture)
        SDL_DestroyTexture(bgTexture);
    if (bgTextureDRC)
        SDL_DestroyTexture(bgTextureDRC);

    if (systemAvatar)
        SDL_DestroyTexture(systemAvatar);

    if (drcRenderer)
        SDL_DestroyRenderer(drcRenderer);
    if (drcWindow)
        SDL_DestroyWindow(drcWindow);
    if (tvRenderer)
        SDL_DestroyRenderer(tvRenderer);
    if (tvWindow)
        SDL_DestroyWindow(tvWindow);

    DestroyAvatars();
    IMG_Quit();
    FreeFonts();
    TTF_Quit();
    romfsExit();
    SDL_CloseAudio();
    SDL_Quit();
    WHBProcShutdown();
    return 0;
}
