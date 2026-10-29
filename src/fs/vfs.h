#pragma once

#include <stddef.h>

namespace Vfs {

bool begin();
bool mounted();
const char *cwd();
bool resolve(const char *in, char *out, size_t cap);
bool parentOf(const char *abs, char *out, size_t cap);
bool isDir(const char *abs);
bool ls(const char *path);
bool cd(const char *path);
bool cat(const char *path);
bool makeDir(const char *path);
bool removeDir(const char *path);
bool rm(const char *path);
bool cp(const char *src, const char *dst);
bool mv(const char *src, const char *dst);
void showMotd();

}  // namespace Vfs
