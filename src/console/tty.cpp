#include "console/tty.h"

#include <string.h>

namespace {

#if defined(ARDUINO_ARCH_ESP32)
constexpr int kHist = 8;
#else
constexpr int kHist = 2;
#endif

char gHist[kHist][Tty::kCap];
char gStash[Tty::kCap];
int gHistCount = 0;
int gHistTail = 0;
int gHistView = -1;
int gHistWalk = 0;

void rawBytes(const uint8_t *data, size_t len) {
  size_t off = 0;
  const uint32_t t0 = millis();
  while (off < len && (millis() - t0) < 40) {
    const size_t n = Serial.write(data + off, len - off);
    if (n == 0) {
      delay(1);
      continue;
    }
    off += n;
  }
}

void rawStr(const char *s) {
  if (s && *s) {
    rawBytes(reinterpret_cast<const uint8_t *>(s), strlen(s));
  }
}

void rawByte(uint8_t b) { rawBytes(&b, 1); }

}  // namespace

Tty tty;

void Tty::begin() {
  _len = 0;
  _cursor = 0;
  _line[0] = '\0';
  _secret = false;
  _eofLogsOut = false;
  _swallowLf = false;
  _esc = Esc::None;
  _csi = 0;
  _fn = nullptr;
  strncpy(_prompt, "> ", sizeof(_prompt) - 1);
  _prompt[sizeof(_prompt) - 1] = '\0';
  gHistCount = 0;
  gHistTail = 0;
  gHistView = -1;
  gHistWalk = 0;
  memset(gHist, 0, sizeof(gHist));
  gStash[0] = '\0';
}

void Tty::setPrompt(const char *prompt) {
  if (!prompt) {
    prompt = "> ";
  }
  strncpy(_prompt, prompt, sizeof(_prompt) - 1);
  _prompt[sizeof(_prompt) - 1] = '\0';
}

void Tty::setSecret(bool on) {
  _secret = on;
  if (on) {
    _cursor = _len;
  }
}

void Tty::setEofLogsOut(bool on) { _eofLogsOut = on; }

void Tty::setCompleter(Completer fn) { _fn = fn; }

void Tty::discard() {
  memset(_line, 0, sizeof(_line));
  _len = 0;
  _cursor = 0;
  _esc = Esc::None;
  _swallowLf = false;
  gHistView = -1;
  gHistWalk = 0;
}

void Tty::remember(const char *line) {
  if (!line || !line[0]) {
    return;
  }
  if (gHistCount > 0) {
    const int newest = (gHistTail + kHist - 1) % kHist;
    if (strcmp(gHist[newest], line) == 0) {
      return;
    }
  }
  strncpy(gHist[gHistTail], line, kCap - 1);
  gHist[gHistTail][kCap - 1] = '\0';
  gHistTail = (gHistTail + 1) % kHist;
  if (gHistCount < kHist) {
    gHistCount++;
  }
}

void Tty::redraw() {
  rawByte('\r');
  rawStr(_prompt);
  if (!_secret && _len > 0) {
    rawBytes(reinterpret_cast<const uint8_t *>(_line), _len);
  }
  rawStr("\033[K");
  if (!_secret) {
    for (size_t i = _cursor; i < _len; i++) {
      rawByte('\b');
    }
  }
}

void Tty::printPrompt() {
  _len = 0;
  _cursor = 0;
  _line[0] = '\0';
  _esc = Esc::None;
  gHistView = -1;
  gHistWalk = 0;
  redraw();
}

void Tty::clearScreen() {
  // CSI clear for terminals that honor it, then blank lines so a monitor
  // that ignores escape sequences still scrolls the old text off screen.
  rawStr("\033[2J\033[3J\033[H\f");
  char blank[81];
  memset(blank, '\n', 80);
  blank[80] = '\0';
  rawStr(blank);
  redraw();
}

void Tty::loadLine(const char *src) {
  strncpy(_line, src ? src : "", kCap - 1);
  _line[kCap - 1] = '\0';
  _len = strlen(_line);
  _cursor = _len;
  redraw();
}

void Tty::older() {
  if (_secret || gHistCount == 0) {
    return;
  }
  if (gHistView < 0) {
    strncpy(gStash, _line, kCap - 1);
    gStash[kCap - 1] = '\0';
    gHistView = (gHistTail + kHist - 1) % kHist;
    gHistWalk = 1;
  } else if (gHistWalk < gHistCount) {
    gHistView = (gHistView + kHist - 1) % kHist;
    gHistWalk++;
  } else {
    return;
  }
  loadLine(gHist[gHistView]);
}

void Tty::newer() {
  if (_secret || gHistView < 0) {
    return;
  }
  if (gHistWalk <= 1) {
    gHistView = -1;
    gHistWalk = 0;
    loadLine(gStash);
    return;
  }
  gHistView = (gHistView + 1) % kHist;
  gHistWalk--;
  loadLine(gHist[gHistView]);
}

void Tty::killWord() {
  if (_secret) {
    _cursor = _len;
  }
  if (_cursor == 0) {
    return;
  }
  size_t end = _cursor;
  while (_cursor > 0 && _line[_cursor - 1] == ' ') {
    _cursor--;
  }
  while (_cursor > 0 && _line[_cursor - 1] != ' ') {
    _cursor--;
  }
  memmove(_line + _cursor, _line + end, _len - end);
  _len -= (end - _cursor);
  _line[_len] = '\0';
  redraw();
}

Tty::Event Tty::feed(uint8_t c, char *out, size_t outCap) {
  if (_esc != Esc::None) {
    if (_esc == Esc::Esc && c != '[' && c != 'O' && c != 0x1B) {
      _esc = Esc::None;
    } else if (_esc == Esc::Esc) {
      if (c == '[') {
        _esc = Esc::Csi;
        _csi = 0;
      } else if (c == 'O') {
        _esc = Esc::Ss3;
      }
      return Event::Activity;
    } else if (_esc == Esc::Ss3) {
      _esc = Esc::None;
      if (!_secret && c == 'A') older();
      else if (!_secret && c == 'B') newer();
      else if (!_secret && c == 'C' && _cursor < _len) {
        _cursor++;
        redraw();
      } else if (!_secret && c == 'D' && _cursor > 0) {
        _cursor--;
        redraw();
      } else if (!_secret && c == 'H') {
        _cursor = 0;
        redraw();
      } else if (!_secret && c == 'F') {
        _cursor = _len;
        redraw();
      }
      return Event::Activity;
    } else if (_esc == Esc::Csi) {
      if (c >= '0' && c <= '9') {
        _csi = _csi * 10 + (c - '0');
        if (_csi > 200) {
          _esc = Esc::None;
        }
        return Event::Activity;
      }
      if (c == ';') {
        _csi = 0;
        return Event::Activity;
      }
      _esc = Esc::None;
      if (_secret) {
        return Event::Activity;
      }
      if (c == 'A') older();
      else if (c == 'B') newer();
      else if (c == 'C' && _cursor < _len) {
        _cursor++;
        redraw();
      } else if (c == 'D' && _cursor > 0) {
        _cursor--;
        redraw();
      } else if (c == 'H') {
        _cursor = 0;
        redraw();
      } else if (c == 'F') {
        _cursor = _len;
        redraw();
      } else if (c == '~') {
        if (_csi == 3 && _cursor < _len) {
          memmove(_line + _cursor, _line + _cursor + 1, _len - _cursor - 1);
          _len--;
          _line[_len] = '\0';
          redraw();
        } else if (_csi == 1 || _csi == 7) {
          _cursor = 0;
          redraw();
        } else if (_csi == 4 || _csi == 8) {
          _cursor = _len;
          redraw();
        }
      }
      return Event::Activity;
    }
  }

  if (c == 0x1B) {
    _esc = Esc::Esc;
    _escMs = millis();
    _csi = 0;
    return Event::Activity;
  }

  if (c == '\n' && _swallowLf) {
    _swallowLf = false;
    return Event::Activity;
  }

  if (c == '\r' || c == '\n') {
    _swallowLf = (c == '\r');
    rawStr("\r\n");
    if (out && outCap > 0) {
      size_t n = _len;
      if (n >= outCap) {
        n = outCap - 1;
      }
      memcpy(out, _line, n);
      out[n] = '\0';
    }
    if (_secret) {
      memset(_line, 0, sizeof(_line));
    }
    _len = 0;
    _cursor = 0;
    gHistView = -1;
    gHistWalk = 0;
    return Event::Line;
  }

  if (c == 0x03) {
    _esc = Esc::None;
    rawStr("^C\r\n");
    memset(_line, 0, sizeof(_line));
    _len = 0;
    _cursor = 0;
    gHistView = -1;
    gHistWalk = 0;
    return Event::Interrupt;
  }

  if (c == 0x04) {
    if (_len == 0 && _eofLogsOut && !_secret) {
      rawStr("\r\n");
      return Event::Logout;
    }
    if (!_secret && _cursor < _len) {
      memmove(_line + _cursor, _line + _cursor + 1, _len - _cursor - 1);
      _len--;
      _line[_len] = '\0';
      redraw();
    }
    return Event::Activity;
  }

  if (c == 0x15) {
    _len = 0;
    _cursor = 0;
    _line[0] = '\0';
    redraw();
    return Event::Activity;
  }

  if (c == 0x0B) {
    _len = _cursor;
    _line[_len] = '\0';
    redraw();
    return Event::Activity;
  }

  if (c == 0x17) {
    killWord();
    return Event::Activity;
  }

  if (c == 0x01) {
    if (!_secret) {
      _cursor = 0;
      redraw();
    }
    return Event::Activity;
  }

  if (c == 0x05) {
    _cursor = _len;
    redraw();
    return Event::Activity;
  }

  if (c == 0x02) {
    if (!_secret && _cursor > 0) {
      _cursor--;
      redraw();
    }
    return Event::Activity;
  }

  if (c == 0x06) {
    if (!_secret && _cursor < _len) {
      _cursor++;
      redraw();
    }
    return Event::Activity;
  }

  if (c == 0x0C) {
    clearScreen();
    return Event::Activity;
  }

  if (c == '\t') {
    if (!_secret && _fn) {
      _fn(_line, kCap, &_len, &_cursor);
      if (_cursor > _len) {
        _cursor = _len;
      }
      _line[_len] = '\0';
    }
    redraw();
    return Event::Activity;
  }

  if (c == 0x08 || c == 0x7F) {
    if (_cursor == 0) {
      return Event::Activity;
    }
    memmove(_line + _cursor - 1, _line + _cursor, _len - _cursor);
    _cursor--;
    _len--;
    _line[_len] = '\0';
    redraw();
    return Event::Activity;
  }

  if (c < 0x20 || c > 0x7E) {
    return Event::Activity;
  }

  if (_len + 1 >= kCap) {
    rawByte('\a');
    return Event::Activity;
  }

  memmove(_line + _cursor + 1, _line + _cursor, _len - _cursor);
  _line[_cursor] = static_cast<char>(c);
  _len++;
  _cursor++;
  _line[_len] = '\0';
  redraw();
  return Event::Activity;
}

Tty::Event Tty::poll(char *out, size_t outCap) {
  if (_esc != Esc::None && (millis() - _escMs) > 80) {
    _esc = Esc::None;
  }
  Event last = Event::None;
  while (Serial.available()) {
    const int r = Serial.read();
    if (r < 0) {
      break;
    }
    const Event ev = feed(static_cast<uint8_t>(r), out, outCap);
    if (ev == Event::Line || ev == Event::Logout || ev == Event::Interrupt) {
      return ev;
    }
    if (ev == Event::Activity) {
      last = Event::Activity;
    }
  }
  return last;
}
