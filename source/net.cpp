#include "net.h"
#include "chat.h"
#include "image.h"

std::string authError = "";

bool showResponse = false;

SDL_Texture* discordAvatar = nullptr;
SDL_Texture* defaultAvatar = nullptr;

std::string serverRules = "";
bool rulesLoaded = false;

int rulesScrollY = 0;

static std::string g_pending;

void LoadAvatars()
{
    discordAvatar = LoadImage(tvRenderer, "romfs:/res/discord.png");
    defaultAvatar = LoadImage(tvRenderer, "romfs:/res/default.png");
}

void DestroyAvatars()
{
    if (discordAvatar)
        SDL_DestroyTexture(discordAvatar);

    if (defaultAvatar)
        SDL_DestroyTexture(defaultAvatar);
}

static bool SetNonBlocking(int sock)
{
    if (sock < 0) return false;
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) flags = 0;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
}



std::string UrlEncode(const std::string& s)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size());
 
    for (unsigned char c : s)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            out += (char)c;
        else
        {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0xF];
        }
    }
 
    return out;
}
 
std::string UrlDecode(const std::string& s)
{
    auto hexVal = [](char h) -> int {
        if (h >= '0' && h <= '9') return h - '0';
        if (h >= 'a' && h <= 'f') return h - 'a' + 10;
        if (h >= 'A' && h <= 'F') return h - 'A' + 10;
        return -1;
    };
 
    std::string out;
    out.reserve(s.size());
 
    for (size_t i = 0; i < s.size(); i++)
    {
        char c = s[i];
 
        if (c == '%' && i + 2 < s.size())
        {
            int hi = hexVal(s[i + 1]);
            int lo = hexVal(s[i + 2]);
 
            if (hi >= 0 && lo >= 0)
            {
                out += (char)((hi << 4) | lo);
                i += 2;
            }
            else
            {
                out += c;
            }
        }
        else
        {
            out += c;
        }
    }
 
    return out;
}

static std::vector<std::string> SplitFields(const std::string& line)
{
    std::vector<std::string> parts;
    size_t start = 0, end;

    while ((end = line.find('|', start)) != std::string::npos)
    {
        parts.push_back(line.substr(start, end - start));
        start = end + 1;
    }

    if (start < line.size())
        parts.push_back(line.substr(start));

    return parts;
}

static bool PopLine(std::string& outLine)
{
    size_t pos = g_pending.find('\n');
    if (pos == std::string::npos) return false;

    outLine = g_pending.substr(0, pos);
    g_pending.erase(0, pos + 1);
    return true;
}

bool SendAll(int sock, const char* data, size_t len)
{
    size_t total = 0;

    while (total < len)
    {
        ssize_t sent = send(sock, data + total, len - total, 0);

        if (sent <= 0)
            return false;

        total += sent;
    }

    return true;
}

bool SendCommand(int sock, const std::string& cmd, const std::vector<std::string>& args)
{
    if (sock < 0) return false;

    std::string out = cmd;
    out += '|';

    for (auto& a : args)
    {
        out += UrlEncode(a);
        out += '|';
    }

    out += '\n';

    return SendAll(sock, out.c_str(), out.size());
}

bool ReadLineBlocking(int sock, std::string& outLine, int timeoutMs)
{
    if (sock < 0) return false;

    if (PopLine(outLine))
        return true;

    Uint32 startTicks = SDL_GetTicks();
    char buf[512];

    while (true)
    {
        int elapsed = (int)(SDL_GetTicks() - startTicks);
        int remaining = timeoutMs - elapsed;
        if (remaining <= 0) return false;

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval tv;
        tv.tv_sec = remaining / 1000;
        tv.tv_usec = (remaining % 1000) * 1000;

        int ready = select(sock + 1, &readfds, nullptr, nullptr, &tv);
        if (ready <= 0) return false; // timeout or select error

        ssize_t r = recv(sock, buf, sizeof(buf), 0);

        if (r > 0)
        {
            g_pending.append(buf, r);
            if (PopLine(outLine)) return true;
        }
        else if (r == 0)
        {
            return false; // server closed connection
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                continue;
            return false;
        }
    }
}

static std::string DescribeErrorCode(const std::string& code)
{
    if (code == "command_unknown")   return "Server did not recognize that command.";
    if (code == "args_bad")          return "Missing or invalid arguments.";
    if (code == "user_exists")       return "That username is already taken.";
    if (code == "register_failure")  return "Registration failed (invalid username?).";
    if (code == "register_disabled") return "Account registration is disabled on this server.";
    if (code == "bad_login")         return "Incorrect username or password.";
    if (code.rfind("banned", 0) == 0) return "You have been banned.";
    return "Error: " + code;
}


int ConnectToTCPServer()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT_TCP);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        close(sock);
        return -1;
    }

    SetNonBlocking(sock);
    g_pending.clear();

    std::string hello;
    if (!ReadLineBlocking(sock, hello, 5000))
    {
        OSReport("V7: no greeting from server");
        close(sock);
        return -1;
    }

    std::vector<std::string> parts = SplitFields(hello);

    if (parts.empty())
    {
        close(sock);
        return -1;
    }

    if (parts[0] == "ipbanned")
    {
        authError = "You are IP banned from this server.";
        showResponse = true;
        close(sock);
        return -1;
    }

    if (parts[0] != "hello" || parts.size() < 2 || parts[1] != "v7")
    {
        OSReport("V7: unexpected greeting: %s", hello.c_str());
        close(sock);
        return -1;
    }

    return sock;
}

void ReconnectToTCPServer()
{
    int newSock = ConnectToTCPServer();

    if (newSock >= 0)
    {
        sock = newSock;
        connectionLost = false;
    }
}


bool login_account(const std::string& username, const std::string& password)
{
    if (sock < 0) return false;

    if (!SendCommand(sock, "login", { username, password }))
        return false;

    std::string line;
    if (!ReadLineBlocking(sock, line, 5000))
    {
        authError = "No response from server.";
        showResponse = true;
        return false;
    }

    std::vector<std::string> parts = SplitFields(line);
    if (parts.empty())
    {
        authError = "Empty server response.";
        showResponse = true;
        return false;
    }

    if (parts[0] == "ok")
    {
        showResponse = false;
        return true;
    }

    if (parts[0] == "err")
    {
        std::string code = parts.size() > 1 ? parts[1] : "unknown";
        authError = DescribeErrorCode(code);
        showResponse = true;
        return false;
    }

    if (parts[0] == "banned")
    {
        std::string reason = parts.size() > 1 ? UrlDecode(parts[1]) : "";
        authError = reason.empty() ? "You have been banned." : ("Banned: " + reason);
        showResponse = true;
        return false;
    }

    authError = "Unexpected server response.";
    showResponse = true;
    return false;
}

bool create_account(const std::string& username, const std::string& password)
{
    if (sock < 0) return false;

    if (!SendCommand(sock, "register", { username, password }))
        return false;

    std::string line;
    if (!ReadLineBlocking(sock, line, 5000))
    {
        authError = "No response from server.";
        showResponse = true;
        return false;
    }

    std::vector<std::string> parts = SplitFields(line);
    if (parts.empty())
    {
        authError = "Empty server response.";
        showResponse = true;
        return false;
    }

    if (parts[0] == "ok")
    {
        showResponse = false;
        return true;
    }

    if (parts[0] == "err")
    {
        std::string code = parts.size() > 1 ? parts[1] : "unknown";
        authError = DescribeErrorCode(code);
        showResponse = true;
        return false;
    }

    authError = "Unexpected server response.";
    showResponse = true;
    return false;
}


void join_room(const std::string& room)
{
    if (sock < 0) return;

    currentRoom = room;
    SendCommand(sock, "join", { room });
}


void request_rules()
{
    if (sock < 0) return;
    rulesLoaded = false;
    rulesScrollY = 0;
    SendCommand(sock, "rules", {});
}


void request_history(const std::string& size)
{
    if (sock < 0) return;

    if (size.empty())
        SendCommand(sock, "history", {});
    else
        SendCommand(sock, "history", { size });
}


void send_chat(const std::string& message)
{
    if (sock < 0) return;

    SendCommand(sock, "msg", { message });
}

void TryReceive(int* sock, SDL_Renderer* renderer, int fontSize, SDL_Color textColor, int maxWidth)
{
    if (*sock < 0) return;

    char buf[512];

    while (true)
    {
        ssize_t r = recv(*sock, buf, sizeof(buf), 0);

        if (r > 0)
        {
            g_pending.append(buf, r);

            std::string line;
            while (PopLine(line))
            {
                std::vector<std::string> parts = SplitFields(line);
                if (parts.empty()) continue;

                const std::string& cmd = parts[0];

                if (cmd == "msg" && parts.size() >= 3)
                {
                    std::string user = UrlDecode(parts[1]);
                    std::string message = UrlDecode(parts[2]);

                    SDL_Texture* avatar = (user == "auroracross") ? discordAvatar : defaultAvatar;

                    AddChatLine(
                        renderer,
                        user,
                        message,
                        avatar,
                        fontSize,
                        fontSize,
                        textColor,
                        textColor,
                        maxWidth
                    );
                }
                else if (cmd == "err")
                {
                    std::string code = parts.size() > 1 ? parts[1] : "unknown";
                    OSReport("Server error: %s", code.c_str());
                }
                else if (cmd == "ok")
                {
                    OSReport("Server OK: %s", line.c_str());
                }
                else if (cmd == "rules")
                {
                    if (parts.size() > 1)
                        serverRules = UrlDecode(parts[1]);
                    rulesLoaded = true;
                }
                else if (cmd == "motd")
                {
                    if (parts.size() > 1)
                        OSReport("motd: %s", UrlDecode(parts[1]).c_str());
                }
                else
                {
                    OSReport("Unhandled/malformed message: %s", line.c_str());
                }
            }
        }
        else if (r == 0)
        {
            close(*sock);
            *sock = -1;
            g_pending.clear();
            connectionLost = true;
            break;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;

            close(*sock);
            *sock = -1;
            g_pending.clear();
            connectionLost = true;
            break;
        }
    }
}