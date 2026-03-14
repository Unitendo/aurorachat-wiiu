#pragma once
#include <cstdio>
#include <cstring>
#include <string>

void SaveLogin(const std::string& username, const std::string& password);
bool LoadLogin(std::string& username, std::string& password);
void ClearLogin();
