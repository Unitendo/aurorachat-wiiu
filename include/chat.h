#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>

extern bool AutoScrollEnabled;

struct ChatLine {
    std::string username;
    std::string message;
    std::string timestamp;

    // TV
    SDL_Texture* avatarTexture;
    SDL_Texture* nameTexture;
    SDL_Texture* timestampTexture;
    SDL_Texture* messageTexture;
    int nameHeight;
    int messageHeight;

    // DRC
    SDL_Texture* drcAvatarTexture;
    SDL_Texture* drcNameTexture;
    SDL_Texture* drcTimestampTexture;
    SDL_Texture* drcMessageTexture;
    int drcNameHeight;
    int drcMessageHeight;
};

extern std::vector<ChatLine> chatLines;
extern int chatPosY;
extern std::string currentRoom;
extern bool AutoScrollEnabled;

void ScrollChatToBottom(int tvChatViewHeight);

void AddChatLine(SDL_Renderer* tvRenderer, SDL_Renderer* drcRenderer, const std::string& username, const std::string& message, SDL_Texture* tvAvatar, SDL_Texture* drcAvatar, int nameFontSize, int messageFontSize, SDL_Color nameColor, SDL_Color messageColor, int tvMaxWidth, int tvChatViewHeight, int drcMaxWidth, int drcChatViewHeight, const std::string& timestamp = "");

void DrawChatBuffer(SDL_Renderer* renderer, int x, int y);
void DrawLatestMessagesDRC(SDL_Renderer* renderer, int x, int topY, int bottomY);
