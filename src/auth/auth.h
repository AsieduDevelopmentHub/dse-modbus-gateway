#pragma once

#include <stddef.h>

// Local console accounts. On ESP32 these live in NVS as salted SHA-256.
// The USB console asks for them the way an SSH login does. This is not a
// network SSH server.

namespace Auth {

void begin();
bool factoryPassword();
bool login(const char *user, const char *pass);
bool check(const char *user, const char *pass);
const char *user();
void logout();
void printUsers();
bool prepareAdd(const char *name, char *err, size_t errN);
bool addUser(const char *name, const char *pass, char *err, size_t errN);
bool setPassword(const char *name, const char *pass, char *err, size_t errN);
bool delUser(const char *name, char *err, size_t errN);

}  // namespace Auth
