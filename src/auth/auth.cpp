#include "auth/auth.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)

#include <Preferences.h>
#include <ctype.h>
#include <esp_system.h>

#include "mbedtls/md.h"

namespace {

constexpr int kMaxUsers = 4;

struct UserRec {
  char name[16];
  uint8_t salt[8];
  uint8_t hash[32];
};

UserRec gUsers[kMaxUsers];
int gSession = -1;

bool sameBytes(const uint8_t *a, const uint8_t *b, size_t n) {
  uint8_t d = 0;
  for (size_t i = 0; i < n; i++) {
    d |= static_cast<uint8_t>(a[i] ^ b[i]);
  }
  return d == 0;
}

bool hashPass(const uint8_t salt[8], const char *pass, uint8_t out[32]) {
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info || !pass) {
    return false;
  }
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  if (mbedtls_md_setup(&ctx, info, 0) != 0) {
    mbedtls_md_free(&ctx);
    return false;
  }
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, salt, 8);
  mbedtls_md_update(&ctx, reinterpret_cast<const unsigned char *>(pass),
                    strlen(pass));
  mbedtls_md_finish(&ctx, out);
  mbedtls_md_free(&ctx);
  return true;
}

void fillSalt(uint8_t salt[8]) {
  for (int i = 0; i < 8; i++) {
    salt[i] = static_cast<uint8_t>(esp_random() & 0xFFu);
  }
}

int findUser(const char *name) {
  if (!name || !name[0]) {
    return -1;
  }
  for (int i = 0; i < kMaxUsers; i++) {
    if (gUsers[i].name[0] && strcmp(gUsers[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

int userCount() {
  int n = 0;
  for (int i = 0; i < kMaxUsers; i++) {
    if (gUsers[i].name[0]) {
      n++;
    }
  }
  return n;
}

bool validName(const char *name) {
  if (!name) {
    return false;
  }
  const size_t n = strlen(name);
  if (n < 1 || n > 15) {
    return false;
  }
  if (!isalpha(static_cast<unsigned char>(name[0]))) {
    return false;
  }
  for (size_t i = 0; i < n; i++) {
    const unsigned char c = static_cast<unsigned char>(name[i]);
    if (!isalnum(c) && c != '_' && c != '-') {
      return false;
    }
  }
  return true;
}

bool validPass(const char *pass) {
  if (!pass) {
    return false;
  }
  const size_t n = strlen(pass);
  return n >= 4 && n <= 32;
}

bool saveUsers() {
  Preferences prefs;
  if (!prefs.begin("gwauth", false)) {
    return false;
  }
  const size_t n = prefs.putBytes("users", gUsers, sizeof(gUsers));
  prefs.end();
  return n == sizeof(gUsers);
}

bool storePassword(UserRec *rec, const char *pass) {
  uint8_t salt[8];
  uint8_t hash[32];
  fillSalt(salt);
  if (!hashPass(salt, pass, hash)) {
    return false;
  }
  memcpy(rec->salt, salt, 8);
  memcpy(rec->hash, hash, 32);
  return true;
}

void createDefault() {
  memset(gUsers, 0, sizeof(gUsers));
  strncpy(gUsers[0].name, "admin", sizeof(gUsers[0].name) - 1);
  if (!storePassword(&gUsers[0], "admin") || !saveUsers()) {
    Serial.println(F("[auth] could not store the factory account"));
  }
}

bool passwordMatches(const UserRec &rec, const char *pass) {
  uint8_t out[32];
  if (!hashPass(rec.salt, pass ? pass : "", out)) {
    return false;
  }
  return sameBytes(out, rec.hash, 32);
}

}  // namespace

namespace Auth {

void begin() {
  gSession = -1;
  memset(gUsers, 0, sizeof(gUsers));
  Preferences prefs;
  if (!prefs.begin("gwauth", false)) {
    createDefault();
    return;
  }
  const size_t n = prefs.getBytesLength("users");
  if (n == sizeof(gUsers)) {
    prefs.getBytes("users", gUsers, sizeof(gUsers));
  }
  prefs.end();
  if (userCount() == 0) {
    createDefault();
  }
}

bool factoryPassword() { return check("admin", "admin"); }

bool check(const char *user, const char *pass) {
  uint8_t dummySalt[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  uint8_t dummy[32];
  const int i = findUser(user);
  if (i < 0) {
    hashPass(dummySalt, pass ? pass : "", dummy);
    return false;
  }
  return passwordMatches(gUsers[i], pass);
}

bool login(const char *user, const char *pass) {
  if (!check(user, pass)) {
    return false;
  }
  gSession = findUser(user);
  return gSession >= 0;
}

const char *user() {
  if (gSession < 0 || gSession >= kMaxUsers) {
    return "";
  }
  return gUsers[gSession].name;
}

void logout() { gSession = -1; }

void printUsers() {
  Serial.println(F("Users:"));
  for (int i = 0; i < kMaxUsers; i++) {
    if (!gUsers[i].name[0]) {
      continue;
    }
    Serial.print(F("  "));
    Serial.println(gUsers[i].name);
  }
}

bool prepareAdd(const char *name, char *err, size_t errN) {
  if (!validName(name)) {
    snprintf(err, errN, "user name: 1..15, letter first, [A-Za-z0-9_-]");
    return false;
  }
  if (findUser(name) >= 0) {
    snprintf(err, errN, "user exists");
    return false;
  }
  if (userCount() >= kMaxUsers) {
    snprintf(err, errN, "user table full (4)");
    return false;
  }
  return true;
}

bool addUser(const char *name, const char *pass, char *err, size_t errN) {
  if (!prepareAdd(name, err, errN)) {
    return false;
  }
  if (!validPass(pass)) {
    snprintf(err, errN, "password must be 4..32 characters");
    return false;
  }
  int slot = -1;
  for (int i = 0; i < kMaxUsers; i++) {
    if (!gUsers[i].name[0]) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    snprintf(err, errN, "user table full (4)");
    return false;
  }
  memset(&gUsers[slot], 0, sizeof(gUsers[slot]));
  strncpy(gUsers[slot].name, name, sizeof(gUsers[slot].name) - 1);
  if (!storePassword(&gUsers[slot], pass) || !saveUsers()) {
    memset(&gUsers[slot], 0, sizeof(gUsers[slot]));
    snprintf(err, errN, "could not store user");
    return false;
  }
  return true;
}

bool setPassword(const char *name, const char *pass, char *err, size_t errN) {
  const int i = findUser(name);
  if (i < 0) {
    snprintf(err, errN, "unknown user");
    return false;
  }
  if (!validPass(pass)) {
    snprintf(err, errN, "password must be 4..32 characters");
    return false;
  }
  if (!storePassword(&gUsers[i], pass) || !saveUsers()) {
    snprintf(err, errN, "could not store password");
    return false;
  }
  return true;
}

bool delUser(const char *name, char *err, size_t errN) {
  const int i = findUser(name);
  if (i < 0) {
    snprintf(err, errN, "unknown user");
    return false;
  }
  if (userCount() <= 1) {
    snprintf(err, errN, "cannot delete the last user");
    return false;
  }
  if (i == gSession) {
    snprintf(err, errN, "cannot delete the logged-in user");
    return false;
  }
  for (int j = i; j < kMaxUsers - 1; j++) {
    gUsers[j] = gUsers[j + 1];
  }
  memset(&gUsers[kMaxUsers - 1], 0, sizeof(gUsers[0]));
  if (gSession > i) {
    gSession--;
  }
  if (!saveUsers()) {
    snprintf(err, errN, "could not update user store");
    return false;
  }
  return true;
}

}  // namespace Auth

#else

namespace Auth {

void begin() {}
bool factoryPassword() { return false; }
bool login(const char *, const char *) { return false; }
bool check(const char *, const char *) { return false; }
const char *user() { return ""; }
void logout() {}
void printUsers() { Serial.println(F("Accounts are on the ESP32 console.")); }
bool prepareAdd(const char *, char *err, size_t errN) {
  snprintf(err, errN, "Accounts are on the ESP32 console.");
  return false;
}
bool addUser(const char *, const char *, char *err, size_t errN) {
  return prepareAdd(nullptr, err, errN);
}
bool setPassword(const char *, const char *, char *err, size_t errN) {
  return prepareAdd(nullptr, err, errN);
}
bool delUser(const char *, char *err, size_t errN) {
  return prepareAdd(nullptr, err, errN);
}

}  // namespace Auth

#endif
