#pragma once

#include <ctype.h>
#include <stddef.h>

// Portable case-insensitive helpers (AVR Arduino has no strings.h / strcasecmp).

inline int strEqualsIgnoreCase(const char *a, const char *b) {
  if (!a || !b) {
    return (a == b) ? 0 : (a ? 1 : -1);
  }
  while (*a && *b) {
    const int ca = tolower(static_cast<unsigned char>(*a++));
    const int cb = tolower(static_cast<unsigned char>(*b++));
    if (ca != cb) {
      return ca - cb;
    }
  }
  return tolower(static_cast<unsigned char>(*a)) -
         tolower(static_cast<unsigned char>(*b));
}

inline bool strContainsIgnoreCase(const char *haystack, const char *needle) {
  if (!haystack || !needle || !*needle) {
    return false;
  }
  for (const char *h = haystack; *h; ++h) {
    const char *a = h;
    const char *b = needle;
    while (*a && *b &&
           (tolower(static_cast<unsigned char>(*a)) ==
            tolower(static_cast<unsigned char>(*b)))) {
      ++a;
      ++b;
    }
    if (!*b) {
      return true;
    }
  }
  return false;
}
