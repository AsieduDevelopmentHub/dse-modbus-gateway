#include "fs/editor.h"

#include <Arduino.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs/vfs.h"
#include "util/str_util.h"

#if defined(ARDUINO_ARCH_ESP32)

#include <LittleFS.h>

#include "config/settings.h"

namespace {

constexpr uint16_t kCap = 8192;
constexpr int kMaxLines = 180;

char gBuf[kCap];
uint16_t gUsed = 0;
struct Ref {
  uint16_t at;
  uint16_t len;
};
Ref gRef[kMaxLines];
int gCount = 0;
bool gDirty = false;
bool gOpen = false;
char gPath[96];

bool reindex() {
  gCount = 0;
  uint16_t i = 0;
  while (i < gUsed) {
    if (gCount >= kMaxLines) {
      return false;
    }
    const uint16_t start = i;
    while (i < gUsed && gBuf[i] != '\n') {
      i++;
    }
    gRef[gCount].at = start;
    gRef[gCount].len = static_cast<uint16_t>(i - start);
    gCount++;
    if (i < gUsed && gBuf[i] == '\n') {
      i++;
    }
  }
  return true;
}

bool splice(uint16_t at, uint16_t cut, const char *ins, uint16_t insLen) {
  if (at + cut > gUsed) {
    return false;
  }
  if (static_cast<uint32_t>(gUsed) - cut + insLen > kCap) {
    Serial.println(F("File would exceed the editor limit."));
    return false;
  }
  const uint16_t tail = static_cast<uint16_t>(gUsed - (at + cut));
  if (insLen != cut && tail > 0) {
    memmove(gBuf + at + insLen, gBuf + at + cut, tail);
  }
  if (insLen > 0 && ins) {
    memcpy(gBuf + at, ins, insLen);
  }
  gUsed = static_cast<uint16_t>(gUsed - cut + insLen);
  if (!reindex()) {
    Serial.println(F("Too many lines."));
    return false;
  }
  gDirty = true;
  return true;
}

void listLines() {
  for (int i = 0; i < gCount; i++) {
    Serial.print(i + 1);
    Serial.print(F("| "));
    Serial.write(reinterpret_cast<const uint8_t *>(gBuf + gRef[i].at),
                 gRef[i].len);
    Serial.println();
  }
  if (gCount == 0) {
    Serial.println(F("(empty)"));
  }
}

void help() {
  Serial.println(F("l | p          list"));
  Serial.println(F("a <text>       append a line"));
  Serial.println(F("i <n> <text>   insert before line n"));
  Serial.println(F("s <n> <text>   replace line n"));
  Serial.println(F("d <n>          delete line n"));
  Serial.println(F("w              write"));
  Serial.println(F("q              quit if saved"));
  Serial.println(F("q!             quit and discard"));
  Serial.println(F("wq             write and quit"));
}

bool writeBack() {
  if (!Vfs::mounted()) {
    Serial.println(F("Filesystem is not mounted."));
    return false;
  }
  File f = LittleFS.open(gPath, "w");
  if (!f) {
    Serial.println(F("Write failed."));
    return false;
  }
  const size_t n = f.write(reinterpret_cast<const uint8_t *>(gBuf), gUsed);
  f.close();
  if (n != gUsed) {
    Serial.println(F("Write failed."));
    return false;
  }
  gDirty = false;
  Serial.print(F("Wrote "));
  Serial.print(gUsed);
  Serial.println(F(" bytes."));
  return true;
}

char *trim(char *s) {
  while (*s == ' ' || *s == '\t') {
    s++;
  }
  char *e = s + strlen(s);
  while (e > s && (e[-1] == ' ' || e[-1] == '\t')) {
    *--e = '\0';
  }
  return s;
}

bool lineTooLong(const char *text) {
  if (strlen(text) > 160) {
    Serial.println(F("Line too long (160)."));
    return true;
  }
  if (strchr(text, '\n') || strchr(text, '\r')) {
    Serial.println(F("Line cannot contain a break."));
    return true;
  }
  return false;
}

}  // namespace

namespace Editor {

bool open(const char *path) {
  if (!Vfs::mounted()) {
    Serial.println(F("Filesystem is not mounted."));
    return false;
  }
  char full[96];
  if (!Vfs::resolve(path, full, sizeof(full))) {
    Serial.println(F("Bad path."));
    return false;
  }
  if (Vfs::isDir(full)) {
    Serial.println(F("Is a directory."));
    return false;
  }
  gUsed = 0;
  gCount = 0;
  gDirty = false;
  strncpy(gPath, full, sizeof(gPath) - 1);
  gPath[sizeof(gPath) - 1] = '\0';

  if (LittleFS.exists(full)) {
    File f = LittleFS.open(full, "r");
    if (!f) {
      Serial.println(F("Open failed."));
      return false;
    }
    if (f.size() > 8000) {
      f.close();
      Serial.println(F("File is too large to edit (limit 8000 bytes)."));
      return false;
    }
    const int n = f.read(reinterpret_cast<uint8_t *>(gBuf), kCap);
    f.close();
    gUsed = n > 0 ? static_cast<uint16_t>(n) : 0;
    uint16_t w = 0;
    for (uint16_t r = 0; r < gUsed; r++) {
      if (gBuf[r] != '\r') {
        gBuf[w++] = gBuf[r];
      }
    }
    gUsed = w;
  } else {
    char parent[96];
    if (!Vfs::parentOf(full, parent, sizeof(parent)) || !Vfs::isDir(parent)) {
      Serial.println(F("Parent folder does not exist."));
      return false;
    }
    gDirty = true;
    Serial.println(F("New file."));
  }

  if (!reindex()) {
    Serial.println(F("Too many lines to edit."));
    return false;
  }
  gOpen = true;
  Serial.print(F("-- "));
  Serial.print(gPath);
  Serial.print(F(" ("));
  Serial.print(gCount);
  Serial.println(F(" lines) --"));
  listLines();
  Serial.println(F("s <line> <text> replaces a line.  a <text> adds a line.  d <line> deletes."));
  Serial.println(F("wq saves and quits.  q! quits without saving.  l lists again."));
  if (strcmp(gPath, Settings::path()) == 0) {
    Serial.println(F("After saving this file, run: config reload"));
  }
  return true;
}

bool onLine(char *line) {
  if (!gOpen) {
    return false;
  }
  char *s = trim(line);
  if (!s[0]) {
    return true;
  }
  if (strEqualsIgnoreCase(s, "?") == 0 || strEqualsIgnoreCase(s, "help") == 0) {
    help();
    return true;
  }
  if (strEqualsIgnoreCase(s, "l") == 0 || strEqualsIgnoreCase(s, "p") == 0) {
    listLines();
    return true;
  }
  if (strEqualsIgnoreCase(s, "w") == 0) {
    writeBack();
    return true;
  }
  if (strcmp(s, "q!") == 0) {
    gDirty = false;
    gOpen = false;
    return false;
  }
  if (strEqualsIgnoreCase(s, "q") == 0) {
    if (gDirty) {
      Serial.println(F("Not saved. wq to write, q! to discard."));
      return true;
    }
    gOpen = false;
    return false;
  }
  if (strEqualsIgnoreCase(s, "wq") == 0) {
    if (!writeBack()) {
      return true;
    }
    gOpen = false;
    return false;
  }

  char *sp = s;
  while (*sp && *sp != ' ' && *sp != '\t') {
    sp++;
  }
  if (*sp) {
    *sp = '\0';
    sp++;
    while (*sp == ' ' || *sp == '\t') {
      sp++;
    }
  }

  if (strEqualsIgnoreCase(s, "a") == 0) {
    if (lineTooLong(sp)) {
      return true;
    }
    const size_t tl = strlen(sp);
    if (gUsed > 0 && gBuf[gUsed - 1] != '\n') {
      if (gUsed + 1 > kCap) {
        Serial.println(F("File would exceed the editor limit."));
        return true;
      }
      gBuf[gUsed++] = '\n';
    }
    if (static_cast<uint32_t>(gUsed) + tl + 1 > kCap) {
      Serial.println(F("File would exceed the editor limit."));
      return true;
    }
    memcpy(gBuf + gUsed, sp, tl);
    gUsed = static_cast<uint16_t>(gUsed + tl);
    gBuf[gUsed++] = '\n';
    reindex();
    gDirty = true;
    return true;
  }

  if (strEqualsIgnoreCase(s, "d") == 0 || strEqualsIgnoreCase(s, "s") == 0 ||
      strEqualsIgnoreCase(s, "i") == 0) {
    if (!isdigit(static_cast<unsigned char>(*sp))) {
      Serial.println(F("Need a line number."));
      return true;
    }
    char *end = nullptr;
    const long n = strtol(sp, &end, 10);
    if (end == sp) {
      Serial.println(F("Need a line number."));
      return true;
    }
    while (*end == ' ' || *end == '\t') {
      end++;
    }
    const bool del = strEqualsIgnoreCase(s, "d") == 0;
    const bool ins = strEqualsIgnoreCase(s, "i") == 0;
    if (del) {
      if (n < 1 || n > gCount) {
        Serial.println(F("No such line."));
        return true;
      }
      const int i = static_cast<int>(n - 1);
      uint16_t cut = gRef[i].len;
      if (gRef[i].at + cut < gUsed && gBuf[gRef[i].at + cut] == '\n') {
        cut++;
      }
      splice(gRef[i].at, cut, nullptr, 0);
      return true;
    }
    if (lineTooLong(end)) {
      return true;
    }
    const size_t tl = strlen(end);
    char chunk[162];
    memcpy(chunk, end, tl);
    chunk[tl] = '\n';
    if (ins) {
      if (gCount == 0 && n == 1) {
        splice(0, 0, chunk, static_cast<uint16_t>(tl + 1));
        return true;
      }
      if (n < 1 || n > gCount) {
        Serial.println(F("No such line."));
        return true;
      }
      splice(gRef[n - 1].at, 0, chunk, static_cast<uint16_t>(tl + 1));
      return true;
    }
    if (n < 1 || n > gCount) {
      Serial.println(F("No such line."));
      return true;
    }
    splice(gRef[n - 1].at, gRef[n - 1].len, end, static_cast<uint16_t>(tl));
    return true;
  }

  Serial.println(F("Unknown editor command. ? for help."));
  return true;
}

void close() { gOpen = false; }

bool dirty() { return gOpen && gDirty; }

bool active() { return gOpen; }

}  // namespace Editor

#else

namespace Editor {

bool open(const char *) {
  Serial.println(F("The editor is on the ESP32 console."));
  return false;
}
bool onLine(char *) { return false; }
void close() {}
bool dirty() { return false; }
bool active() { return false; }

}  // namespace Editor

#endif
