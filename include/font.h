#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <map>
#include <string>
#include <unordered_map>

TTF_Font* GetFont(int size);
void FreeFonts();

SDL_Texture* DrawTextToTexture(SDL_Renderer* renderer, const char* text, int size, SDL_Color color, int maxWidth, bool bold = false);

SDL_Texture* GetCachedText(SDL_Renderer* renderer, const std::string& text, int size, SDL_Color color, int* outW, int* outH);
void DrawTextCached(SDL_Renderer* renderer, const std::string& text, int x, int y, int size, SDL_Color color);
void ClearTextCache();
