#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// Local line editor for the USB console.
// The host monitor must have local echo off; this firmware draws the line.

class Tty {
public:
  static constexpr size_t kCap = 128;

  enum class Event : uint8_t { None, Activity, Line, Logout, Interrupt };

  using Completer = bool (*)(char *line, size_t cap, size_t *len, size_t *cursor);

  void begin();
  void setPrompt(const char *prompt);
  void setSecret(bool on);
  void setEofLogsOut(bool on);
  void setCompleter(Completer fn);

  Event poll(char *out, size_t outCap);
  void printPrompt();
  void clearScreen();
  void discard();
  void remember(const char *line);

private:
  enum class Esc : uint8_t { None, Esc, Csi, Ss3 };

  Event feed(uint8_t c, char *out, size_t outCap);
  void redraw();
  void older();
  void newer();
  void loadLine(const char *src);
  void killWord();

  char _line[kCap];
  size_t _len = 0;
  size_t _cursor = 0;
  char _prompt[96] = {};
  bool _secret = false;
  bool _eofLogsOut = false;
  bool _swallowLf = false;
  Esc _esc = Esc::None;
  int _csi = 0;
  uint32_t _escMs = 0;
  Completer _fn = nullptr;
};

extern Tty tty;
