#pragma once
#include "esphome.h"

class MainPage {
public:
  explicit MainPage(display::DisplayBuffer &buffer)
    : buffer_(buffer),
      width_(buffer.get_width()),
      height_(buffer.get_height()),
      h_center_{width_ / 2} {}

  void draw() const {
    constexpr int margin = 4;

    buffer_.fill(COLOR_ON);

    char buffer[64];
    size_t ret = id(esptime).now().strftime(buffer, sizeof(buffer), "%H:%M");
    if (ret > 0) {
      int x1, y1, w, h;
      buffer_.get_text_bounds (
        0,
        0,
        buffer,
        &id(medium),
        TextAlign::TOP_LEFT,
        &x1, &y1, &w, &h
      );
      buffer_.print(
        rand() % (width_ - (w + 2 * margin)) + margin,
        rand() % (height_ - (h + 2 * margin)) + margin,
        &id(medium),
        COLOR_OFF,
        TextAlign::TOP_LEFT,
        buffer
      );
    }

    buffer_.strftime(
      margin, 
      width_ - margin, 
      &id(medium), 
      COLOR_OFF, 
      TextAlign::TOP_RIGHT, 
      "%H:%M", 
      id(esptime).now()
    );

  }

private:
  display::DisplayBuffer &buffer_;
  int width_;
  int height_;
  int h_center_;

};
// void draw_main_page(display::DisplayBuffer &buffer) {

//     auto h_center = it.get_width() / 2;


//     if (id(system_status).state) {
//       it.print(700, 100, id(segoeui_48), COLOR_OFF, TextAlign::TOP_RIGHT, "Online");
//     } else {
//       it.print(700, 100, id(segoeui_48), COLOR_OFF, TextAlign::TOP_RIGHT, "Offline");
//     }
     
// }