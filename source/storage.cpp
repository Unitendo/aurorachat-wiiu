#include "storage.h"

static const char* accountFile = "fs:/vol/external01/wiiu/apps/aurorachatforWiiU/account.dat";
static const char* settingsFile = "fs:/vol/external01/wiiu/apps/aurorachatforWiiU/settings.dat";

void SaveLogin(const std::string& username, const std::string& password)
{
    FILE* file = fopen(accountFile, "w");
    if (!file)
        return;


    fprintf(file, "%s\n", username.c_str());
    fprintf(file, "%s\n", password.c_str());

    fclose(file);
}

bool LoadLogin(std::string& username, std::string& password)
{
    FILE* file = fopen(accountFile, "r");
    if (!file)
        return false;

    char user[128];
    char pass[128];

    if (!fgets(user, sizeof(user), file) || !fgets(pass, sizeof(pass), file)) {
        fclose(file);
        return false;
    }

    fclose(file);

    // remove newline
    user[strcspn(user, "\n")] = 0;
    pass[strcspn(pass, "\n")] = 0;

    username = user;
    password = pass;

    return true;
}

void ClearLogin()
{
    remove(accountFile);
}

void SaveSettings(bool autoLoginEnabled)
{
    FILE* file = fopen(settingsFile, "w");
    if (!file)
        return;

    fprintf(file, "%d\n", autoLoginEnabled ? 1 : 0);

    fclose(file);
}

bool LoadSettings(bool& autoLoginEnabled)
{
    FILE* file = fopen(settingsFile, "r");
    if (!file)
        return false;

    char buf[16];
    if (!fgets(buf, sizeof(buf), file)) {
        fclose(file);
        return false;
    }

    fclose(file);

    buf[strcspn(buf, "\n")] = 0;
    autoLoginEnabled = (atoi(buf) != 0);

    return true;
}
