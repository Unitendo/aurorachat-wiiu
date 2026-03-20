#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <string>

SDL_Texture* LoadImage(SDL_Renderer* renderer, const char* path);
SDL_Texture* LoadAvatar(SDL_Renderer* renderer, const std::string& username);
