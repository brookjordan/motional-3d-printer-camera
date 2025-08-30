#include "page_words.h"
#include <FFat.h>
#include <ctype.h>

static inline String trimStr(const String &s) {
  int a = 0, b = s.length();
  while (a < b && isspace((unsigned char)s[a])) a++;
  while (b > a && isspace((unsigned char)s[b - 1])) b--;
  return s.substring(a, b);
}

void loadPageWords(PageWords &out) {
  File f = FFat.open("/you-can-change-this/page-words.txt", "r");
  if (!f) return; // keep defaults
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;
    if (line[0] == '#') continue;
    int colon = line.indexOf(':'); if (colon <= 0) continue;
    String key = trimStr(line.substring(0, colon));
    String val = trimStr(line.substring(colon + 1));
    String keyL = key; keyL.toLowerCase();

    if (keyL.indexOf("page title") >= 0 || keyL.indexOf("title") >= 0) {
      out.pageTitle = val;
    } else if (keyL.indexOf("big heading") >= 0 || keyL.indexOf("heading") >= 0) {
      out.heading = val;
    } else if (keyL.indexOf("help line") >= 0 || keyL.indexOf("help") >= 0 || keyL.indexOf("subtitle") >= 0) {
      out.helpLine = val;
    }
  }
  f.close();
}

