#pragma once
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstddef>
#include <string>

#include <unistd.h>
#include <fcntl.h>

#include <coreinit/debug.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "net.h"
#include "chat.h"

#define SERVER_IP "104.236.25.60"
#define SERVER_PORT_TCP 7070
#define SERVER_PORT_HTTP 7071

// These are defined in main.cpp
extern int sock;
extern SDL_Renderer* tvRenderer;
extern SDL_Renderer* drcRenderer;
extern int fontSize;
extern int maxWidth;
extern SDL_Color tvTextColor;
extern bool connectionLost;

extern std::string authToken;
extern std::string authError;

extern bool showResponse;

extern std::string currentRoom;

extern std::string serverRules;
extern std::string serverMOTD;
extern bool rulesLoaded;
extern bool motdLoaded;

extern int rulesScrollY;
extern int motdScrollY;

extern bool expectingHistory;
extern Uint32 lastHistoryMsgTicks;

void LoadAvatars();
void DestroyAvatars();

int ConnectToTCPServer();
void ReconnectToTCPServer();
int ConnectToHTTPServer();

void TryReceive(int* sock, SDL_Renderer* tvRenderer, SDL_Renderer* drcRenderer, int fontSize, SDL_Color textColor, int tvMaxWidth, int tvChatViewHeight, int drcMaxWidth, int drcChatViewHeight);
std::string send_post_request(const std::string& endpoint, const std::string& body);

bool create_account(const std::string& username, const std::string& password);
bool login_account(const std::string& username, const std::string& password);

void fetch_rooms();
void join_room(const std::string& room);
void part_room();
void request_rules();
void request_motd();
void request_history(const std::string& size);
void send_chat(const std::string& message);
