#include <string>

inline std::string color_text(std::string s, int color_code) {
  return "\x1b[" + std::to_string(color_code) + "m" + s + "\x1b[0m";
}

inline std::string color_red(std::string s) {
  int color_code = 31;
  return color_text(s, color_code);
}

inline std::string color_green(std::string s) {
  int color_code = 32;
  return color_text(s, color_code);
}

inline std::string color_blue(std::string s) {
  int color_code = 36;
  return color_text(s, color_code);
}