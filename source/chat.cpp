#include "chat.h"
#include "font.h"
#include "scale.h"

std::vector<ChatLine> chatLines;
int chatPosY = 0;

std::string currentRoom = "general";

bool AutoScrollEnabled = true;

static int ComputeBottomChatPosY(int tvChatViewHeight)
{
    int totalHeight = 0;
    for (auto& l : chatLines)
    {
        totalHeight += (l.nameHeight - SF(32));
        totalHeight += l.messageHeight;
    }

    int bottomPosY = tvChatViewHeight - totalHeight;
    if (bottomPosY > 0)
        bottomPosY = 0;

    return bottomPosY;
}

static bool IsAtBottom(int tvChatViewHeight)
{
    int bottomPosY = ComputeBottomChatPosY(tvChatViewHeight);

    return chatPosY <= bottomPosY + SY(250); // Allow a small margin of error
}

void ScrollChatToBottom(int tvChatViewHeight)
{
    chatPosY = ComputeBottomChatPosY(tvChatViewHeight);
}

void AddChatLine(SDL_Renderer* tvRenderer, SDL_Renderer* drcRenderer, const std::string& username, const std::string& message, SDL_Texture* tvAvatar, SDL_Texture* drcAvatar, int nameFontSize, int messageFontSize, SDL_Color nameColor, SDL_Color messageColor, int tvMaxWidth, int tvChatViewHeight, int drcMaxWidth, int drcChatViewHeight, const std::string& timestamp)
{
    const int avatarSize = SF(128);
    const int avatarPadding = SF(8);

    const int drcAvatarSize = 64;
    const int drcAvatarPadding = 6;
    const int drcFontSize = 20;

    bool wasAtBottom = IsAtBottom(tvChatViewHeight);

    ChatLine line;
    line.username = username;
    line.message = message;
    line.timestamp = timestamp;
    line.avatarTexture = tvAvatar;
    line.drcAvatarTexture = drcAvatar;

    // TV textures
    int tvTextStartX = avatarSize + avatarPadding;
    int tvWrapWidth = tvMaxWidth - tvTextStartX;

    line.nameTexture = DrawTextToTexture(tvRenderer, username.c_str(), nameFontSize, nameColor, tvWrapWidth, true);

    line.timestampTexture = nullptr;

    if (!timestamp.empty())
        line.timestampTexture = DrawTextToTexture(tvRenderer, timestamp.c_str(), nameFontSize / 2, nameColor, tvWrapWidth, false);

    line.messageTexture = DrawTextToTexture(tvRenderer, message.c_str(), messageFontSize, messageColor, tvWrapWidth, false);

    // DRC textures
    int drcTextStartX = drcAvatarSize + drcAvatarPadding;
    int drcWrapWidth = drcMaxWidth - drcTextStartX;

    line.drcNameTexture = DrawTextToTexture(drcRenderer, username.c_str(), drcFontSize, nameColor, drcWrapWidth, true);

    line.drcTimestampTexture = nullptr;

    if (!timestamp.empty())
        line.drcTimestampTexture = DrawTextToTexture(drcRenderer, timestamp.c_str(), drcFontSize / 2, nameColor, drcWrapWidth, false);

    line.drcMessageTexture = DrawTextToTexture(drcRenderer, message.c_str(), drcFontSize, messageColor, drcWrapWidth, false);

    if (!line.nameTexture || !line.messageTexture || !line.drcNameTexture || !line.drcMessageTexture || (!timestamp.empty() && (!line.timestampTexture || !line.drcTimestampTexture)))
        return;

    int w, h;
    SDL_QueryTexture(line.nameTexture, nullptr, nullptr, &w, &h);
    line.nameHeight = h;

    SDL_QueryTexture(line.messageTexture, nullptr, nullptr, &w, &h);
    line.messageHeight = h;

    SDL_QueryTexture(line.drcNameTexture, nullptr, nullptr, &w, &h);
    line.drcNameHeight = h;

    SDL_QueryTexture(line.drcMessageTexture, nullptr, nullptr, &w, &h);
    line.drcMessageHeight = h;

    chatLines.push_back(line);

    // Only auto-scroll if the user was at the bottom
    if (AutoScrollEnabled && wasAtBottom)
    {
        ScrollChatToBottom(tvChatViewHeight);
    }
}

void DrawChatBuffer(SDL_Renderer* renderer, int x, int y)
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

        // Draw timestamp beside username
        if (line.timestampTexture)
        {
            int timestampW, timestampH;

            SDL_QueryTexture(line.timestampTexture, nullptr, nullptr, &timestampW, &timestampH);

            const int timestampPadding = SF(16);

            int timestampY = drawY + (h - timestampH) / 2 - SF(10);
            SDL_Rect timestampRect = { textStartX + w + timestampPadding, timestampY, timestampW, timestampH };

            SDL_RenderCopy(renderer, line.timestampTexture, nullptr, &timestampRect);
        }

        drawY += h - messageSpacing; // spacing between avatar and message

        // Draw message
        SDL_QueryTexture(line.messageTexture, nullptr, nullptr, &w, &h);
        SDL_Rect msgRect = { textStartX, drawY, w, h };
        SDL_RenderCopy(renderer, line.messageTexture, nullptr, &msgRect);

        drawY += h; // spacing between new messages
    }
}

void DrawLatestMessagesDRC(SDL_Renderer* renderer, int x, int topY, int bottomY)
{
    if (chatLines.empty())
        return;

    const int avatarSize = 48;
    const int avatarPadding = 8;
    const int messageSpacing = 12;

    int textStartX = x + avatarSize + avatarPadding;

    SDL_Rect clipRect = { 0, topY, 854, bottomY - topY };
    SDL_RenderSetClipRect(renderer, &clipRect);

    int drawBottom = bottomY;
    std::vector<ChatLine*> toDraw;

    for (auto it = chatLines.rbegin(); it != chatLines.rend(); ++it)
    {
        ChatLine& line = *it;

        int nameH, msgH;
        SDL_QueryTexture(line.drcNameTexture, nullptr, nullptr, nullptr, &nameH);
        SDL_QueryTexture(line.drcMessageTexture, nullptr, nullptr, nullptr, &msgH);

        int blockHeight = std::max(avatarSize, (nameH - messageSpacing) + msgH);

        if (drawBottom - blockHeight < topY)
            break; // no more room

        toDraw.push_back(&line);
        drawBottom -= (blockHeight);
    }

    int drawY = drawBottom;

    for (auto it = toDraw.rbegin(); it != toDraw.rend(); ++it)
    {
        ChatLine& line = **it;
        int w, h;

        if (line.drcAvatarTexture) {
            SDL_Rect avatarRect = { x, drawY, avatarSize, avatarSize };
            SDL_RenderCopy(renderer, line.drcAvatarTexture, nullptr, &avatarRect);
        }

        SDL_QueryTexture(line.drcNameTexture, nullptr, nullptr, &w, &h);
        SDL_Rect nameRect = { textStartX, drawY, w, h };
        SDL_RenderCopy(renderer, line.drcNameTexture, nullptr, &nameRect);

        // Draw timestamp beside username
        if (line.drcTimestampTexture)
        {
            int timestampW, timestampH;

            SDL_QueryTexture(line.drcTimestampTexture, nullptr, nullptr, &timestampW, &timestampH);

            const int timestampPadding = 8;

            int timestampY = drawY + (h - timestampH) / 2 - 3;
            SDL_Rect timestampRect = { textStartX + w + timestampPadding, timestampY, timestampW, timestampH };

            SDL_RenderCopy(renderer, line.drcTimestampTexture, nullptr, &timestampRect);
        }

        int msgY = drawY + h - messageSpacing;

        SDL_QueryTexture(line.drcMessageTexture, nullptr, nullptr, &w, &h);
        SDL_Rect msgRect = { textStartX, msgY, w, h };
        SDL_RenderCopy(renderer, line.drcMessageTexture, nullptr, &msgRect);

        int blockHeight = std::max(avatarSize, (nameRect.h - messageSpacing) + h);
        drawY += blockHeight;
    }

    SDL_RenderSetClipRect(renderer, nullptr);
}
