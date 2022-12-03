#pragma once
#include "esphome.h"

class MainPage {
public:
  explicit MainPage(display::DisplayBuffer &buffer)
    : buffer_(buffer), width_(buffer.get_width()), h_center_{width_ / 2} {}

  void draw() const {
    constexpr int margin = 4;

    buffer_.fill(COLOR_ON);
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