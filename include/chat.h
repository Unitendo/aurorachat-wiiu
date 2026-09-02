#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>

extern bool AutoScrollEnabled;

struct ChatLine
{
    std::string username;
    std::string message;
    std::string room;

    SDL_Texture* nameTexture;
    SDL_Texture* messageTexture;
    SDL_Texture* avatarTexture;

    int nameHeight;
    int messageHeight;
    int avatarSize;

    // Constructor
    ChatLine()
        : nameTexture(nullptr)
        , messageTexture(nullptr)
        , avatarTexture(nullptr)
        , nameHeight(0)
        , messageHeight(0)
        , avatarSize(48)
    {}
};

struct Room {
    char name[64];
    char description[256];
};

extern std::string currentRoom;

extern std::vector<ChatLine> chatLines;
extern int chatPosY;

void AddChatLine(SDL_Renderer* renderer, const std::string& username, const std::string& message, SDL_Texture* avatar, int nameFontSize, int messageFontSize, SDL_Color nameColor, SDL_Color messageColor, int maxWidth, int chatViewHeight);

void DrawChatBuffer(SDL_Renderer* renderer, int x, int y);
