#include "chat.h"
#include "font.h"

std::vector<ChatLine> chatLines;
int chatPosY = 0;

void AddChatLine(SDL_Renderer* renderer,
                 const std::string& username,
                 const std::string& message,
                 SDL_Texture* avatar,
                 int nameFontSize,
                 int messageFontSize,
                 SDL_Color nameColor,
                 SDL_Color messageColor,
                 int maxWidth)
{
    const int avatarSize = 128;
    const int avatarPadding = 8;

    ChatLine line;
    line.username = username;
    line.message = message;
    line.avatarTexture = avatar;

    // Username texture
    int textStartX = avatarSize + avatarPadding;
    int wrapWidth = maxWidth - textStartX;

    line.nameTexture = DrawTextToTexture(
        renderer,
        username.c_str(),
        nameFontSize,
        nameColor,
        wrapWidth,
        true // bold
    );

    // Message texture (wraps)
    line.messageTexture = DrawTextToTexture(
        renderer,
        message.c_str(),
        messageFontSize,
        messageColor,
        wrapWidth,
        false // not bold
    );

    if (!line.nameTexture || !line.messageTexture)
        return;

    int w, h;
    SDL_QueryTexture(line.nameTexture, nullptr, nullptr, &w, &h);
    line.nameHeight = h;

    SDL_QueryTexture(line.messageTexture, nullptr, nullptr, &w, &h);
    line.messageHeight = h;

    chatLines.push_back(line);
}

void DrawChatBuffer(SDL_Renderer* renderer, int x, int y)
{
    const int avatarSize = 128;
    const int avatarPadding = 8;

    int drawY = y + chatPosY;

    for (auto& line : chatLines)
    {
        int w, h;

        // Draw avatar
        if (line.avatarTexture) {
            SDL_Rect avatarRect = { x, drawY, avatarSize, avatarSize };
            SDL_RenderCopy(renderer, line.avatarTexture, nullptr, &avatarRect);
        }

        int textStartX = x + avatarSize + avatarPadding;

        // Draw username
        SDL_QueryTexture(line.nameTexture, nullptr, nullptr, &w, &h);
        SDL_Rect nameRect = { textStartX, drawY, w, h };
        SDL_RenderCopy(renderer, line.nameTexture, nullptr, &nameRect);

        drawY += h - 32; // spacing between avatar and message

        // Draw message
        SDL_QueryTexture(line.messageTexture, nullptr, nullptr, &w, &h);
        SDL_Rect msgRect = { textStartX, drawY, w, h };
        SDL_RenderCopy(renderer, line.messageTexture, nullptr, &msgRect);

        drawY += h; // spacing between new messages
    }
}

void FreeChatTextures()
{
    for (auto& line : chatLines)
    {
        if (line.nameTexture)
            SDL_DestroyTexture(line.nameTexture);

        if (line.messageTexture)
            SDL_DestroyTexture(line.messageTexture);

        if (line.avatarTexture)
            SDL_DestroyTexture(line.avatarTexture);
    }

    chatLines.clear();
}

void RebuildChatTextures(SDL_Renderer* renderer,
                         int nameFontSize,
                         int messageFontSize,
                         SDL_Color nameColor,
                         SDL_Color messageColor,
                         int maxWidth)
{
    for (auto& line : chatLines)
    {
        // Destroy old textures
        if (line.nameTexture)
            SDL_DestroyTexture(line.nameTexture);

        if (line.messageTexture)
            SDL_DestroyTexture(line.messageTexture);

        // Rebuild username
        line.nameTexture = DrawTextToTexture(
            renderer,
            line.username.c_str(),
            nameFontSize,
            nameColor,
            maxWidth
        );

        // Rebuild message
        line.messageTexture = DrawTextToTexture(
            renderer,
            line.message.c_str(),
            messageFontSize,
            messageColor,
            maxWidth - 60
        );

        // Update sizes
        int w, h;

        SDL_QueryTexture(line.nameTexture, nullptr, nullptr, &w, &h);
        line.nameHeight = h;

        SDL_QueryTexture(line.messageTexture, nullptr, nullptr, &w, &h);
        line.messageHeight = h;
    }
}
