#include "font.h"

static std::map<int, TTF_Font*> g_FontCache;
static const char* g_FontPath = "romfs:/res/FOT-RodinNTLG Pro DB.otf";

struct CachedTextEntry {
    SDL_Texture* texture;
    int w, h;
};

static std::unordered_map<std::string, CachedTextEntry> g_TextCache;

static std::string MakeCacheKey(SDL_Renderer* renderer, const std::string& text, int size, SDL_Color color)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%p|%d|%d,%d,%d,%d|", (void*)renderer, size, color.r, color.g, color.b, color.a);
    return std::string(buf) + text;
}

SDL_Texture* GetCachedText(SDL_Renderer* renderer, const std::string& text, int size, SDL_Color color, int* outW, int* outH)
{
    std::string key = MakeCacheKey(renderer, text, size, color);

    auto it = g_TextCache.find(key);
    if (it != g_TextCache.end())
    {
        if (outW) *outW = it->second.w;
        if (outH) *outH = it->second.h;
        return it->second.texture;
    }

    TTF_Font* font = GetFont(size);
    if (!font) return nullptr;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    int w = surface->w;
    int h = surface->h;
    SDL_FreeSurface(surface);

    if (!texture) return nullptr;

    g_TextCache[key] = { texture, w, h };

    if (outW) *outW = w;
    if (outH) *outH = h;
    return texture;
}

void DrawTextCached(SDL_Renderer* renderer, const std::string& text, int x, int y, int size, SDL_Color color)
{
    int w, h;
    SDL_Texture* texture = GetCachedText(renderer, text, size, color, &w, &h);
    if (!texture) return;

    SDL_Rect dst = { x, y, w, h };
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
}

void ClearTextCache()
{
    for (auto& kv : g_TextCache)
        SDL_DestroyTexture(kv.second.texture);
    g_TextCache.clear();
}

TTF_Font* GetFont(int size) {
    if (g_FontCache.count(size))
        return g_FontCache[size];

    TTF_Font* font = TTF_OpenFont(g_FontPath, size);
    if (!font) return nullptr;

    g_FontCache[size] = font;
    return font;
}

void FreeFonts() {
    for (auto& f : g_FontCache)
        TTF_CloseFont(f.second);
    g_FontCache.clear();
}

SDL_Texture* DrawTextToTexture(SDL_Renderer* renderer, const char* text, int size, SDL_Color color, int maxWidth, bool bold)
{
    TTF_Font* font = GetFont(size);
    if (!font) return nullptr;

    if (bold) {
        TTF_SetFontStyle(font, TTF_STYLE_BOLD);
    } else {
        TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, text, color, maxWidth);
    if (!surface) return nullptr;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}
