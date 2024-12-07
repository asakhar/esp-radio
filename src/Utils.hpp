#pragma once
#include <Arduino.h>

static inline String decodePecent(String in) {
  String res;
  int p = 0;
  int i = in.indexOf('%');
  for (; i != -1 && i + 1 < static_cast<int>(in.length());
       p = i, i = in.indexOf('%', i)) {
    res += in.substring(p, i);
    if (in[i + 1] == '%') {
      res += '%';
      i += 2;
      continue;
    }

    if (isHexadecimalDigit(in[i + 1]) && isHexadecimalDigit(in[i + 2])) {
      auto hexNum = in.substring(i + 1, i + 3);
      char hex = strtoul(hexNum.c_str(), NULL, 16);
      res += hex;
      i += 3;
      continue;
    }
    i += 1;
    res += '%';
  }
  if (p < static_cast<int>(in.length())) {
    res += in.substring(p);
  }
  return res;
}
