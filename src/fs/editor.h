#pragma once

namespace Editor {

bool open(const char *path);
bool onLine(char *line);
void close();
bool dirty();
bool active();

}  // namespace Editor
