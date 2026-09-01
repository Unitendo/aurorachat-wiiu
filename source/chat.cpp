#include "chat.h"
#include "font.h"
#include "scale.h"

std::vector<ChatLine> chatLines;
int chatPosY = 0;

std::string currentRoom = "general";

bool AutoScrollEnabled = true;

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

    // Auto-scroll to bottom
    if (AutoScrollEnabled)
    {
        const int CHAT_VIEW_HEIGHT = 1050;

        int totalHeight = 0;

        for (auto& l : chatLines)
        {
            totalHeight += (l.nameHeight - SF(32));
            totalHeight += l.messageHeight;
        }

        chatPosY = CHAT_VIEW_HEIGHT - totalHeight;

        if (chatPosY > 0)
            chatPosY = 0;
    }
}

void DrawChatBuffer(SDL_Renderer* renderer, int x, int y, float scaleX, float scaleY)
{
    const int avatarSize = SF(128);
    const int avatarPadding = SF(8);
    const int messageSpacing = SF(32);

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

        drawY += h - messageSpacing; // spacing between avatar and message

        // Draw message
        SDL_QueryTexture(line.messageTexture, nullptr, nullptr, &w, &h);
        SDL_Rect msgRect = { textStartX, drawY, w, h };
        SDL_RenderCopy(renderer, line.messageTexture, nullptr, &msgRect);
        
        drawY += h; // spacing between new messages
    }
}
