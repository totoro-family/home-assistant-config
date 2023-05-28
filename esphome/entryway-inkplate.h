#pragma once
#include "esphome.h"

constexpr const char *TextNA = "N/A";
constexpr const char *TextOpen = "Open";
constexpr const char *TextClosed = "Closed";

class LayoutContext {
public:

  LayoutContext(
    display::DisplayBuffer &buffer,
    display::Rect rect,
    int horizontal_margin = 14,
    int line_spacing = 4
  ) 
    : buffer_{buffer},
        rect_{rect},
        line_spacing_{line_spacing} {
    rect_.expand(-horizontal_margin, 0);
  }
 
  void draw_left(const char *text) {
    draw_aliged(text, false);
  }

  void draw_right(const char *text) {
    draw_aliged(text, true);
  }

  void new_line() {
    if (current_line_height_ > 0) {
      const auto h = current_line_height_ + line_spacing_;
      rect_.y += h;
      rect_.h -= h;
      current_line_height_ = 0;
    }
  }

private:
  void draw_aliged(const char *text, bool align_right) {
    int x1, y1, w, h;

    buffer_.get_text_bounds(
      0, 0,
      text,
      &id(default_font),
      TextAlign::TOP_LEFT,
      &x1, &y1, &w, &h);

    buffer_.start_clipping(rect_);

    buffer_.print(
      align_right ? rect_.x2() : rect_.x, 
      rect_.y,
      &id(default_font),
      COLOR_OFF,
      align_right ? TextAlign::TOP_RIGHT : TextAlign::TOP_LEFT,
      text
    );

    buffer_.end_clipping();

    if (h > current_line_height_) {
      current_line_height_ = h;
    }
  }

private:
  display::DisplayBuffer &buffer_;
  display::Rect rect_;
  int line_spacing_;
  int current_line_height_ = 0;

};

struct SensorValue {
public:
  std::string text;
  bool is_ok = false;
public:
  SensorValue(std::string text, bool is_ok)
    : text{std::move(text)}, is_ok{is_ok} {}
};

class SensorModel {
public:
  SensorModel() {
    add_open_closed(id(gallery_window));
    add_open_closed(id(guestroom_windows));
    add_open_closed(id(bedroom_windows));
    add_open_closed(id(bathroom_window));
    add_open_closed(id(dining_room_windows));
    add_open_closed(id(living_room_windows));
    add_open_closed(id(fridge_door));
    add_text_sensor(id(oven));
    add_text_sensor(id(cooktop));
  }

  bool is_all_ok() const {
    for (const auto& p : sensors_) {
      if (!p.second.is_ok) {
        return false;
      }
    }
    return !sensors_.empty();
  }

  const std::map<std::string, SensorValue> &get_sensors() const {
    return sensors_;
  }

private:
  void add(std::string name, std::string value, bool is_ok) {
    sensors_.insert(
      std::make_pair(
        std::move(name),
        SensorValue(std::move(value), is_ok)
      )
    );
  }

  void add_open_closed(binary_sensor::BinarySensor &sensor) {
    const char *text = sensor.has_state()
      ? sensor.state ? TextOpen : TextClosed
      : TextNA;
    bool is_ok = sensor.has_state() && !sensor.state;
    add(sensor.get_name(), text, is_ok);
  }
  
  void add_text_sensor(text_sensor::TextSensor &sensor) {
    std::string text = sensor.has_state()
      ? sensor.state : TextNA;
    bool is_ok = sensor.has_state() && sensor.state == "off";
    add(sensor.get_name(), std::move(text), is_ok);
  }

private:
  std::map<std::string, SensorValue> sensors_;
};

class MainPage {
public:
  explicit MainPage(display::DisplayBuffer &buffer)
      : buffer_(buffer),
        rect_{0, 0, buffer.get_width(), buffer.get_height()} {
    rect_.expand(-H_MARGIN, -LINE_SPACING);
  }

  void draw() {
    buffer_.fill(COLOR_ON);
    LayoutContext layout{ buffer_, rect_ };

    char time[64] = "";
    size_t ret = id(esptime).now().strftime(time, sizeof(time), "%H:%M");

    layout.draw_right(time);
    layout.new_line();

    SensorModel model;
    if (model.is_all_ok()) {
      int y = rect_.y2() - id(totoro).get_height();
      buffer_.image(0, y, &id(totoro));
    } else {
      for (const auto& p : model.get_sensors()) {
        layout.draw_left(p.first.c_str());
        layout.draw_right(p.second.text.c_str());
        layout.new_line();
      }
    }
  }

private:
  display::DisplayBuffer &buffer_;
  display::Rect rect_;

  static constexpr int H_MARGIN = 12;
  static constexpr int LINE_SPACING = 4;

};
// void draw_main_page(display::DisplayBuffer &buffer) {

//     auto h_center = it.get_width() / 2;


//     if (id(system_status).state) {
//       it.print(700, 100, id(segoeui_48), COLOR_OFF, TextAlign::TOP_RIGHT, "Online");
//     } else {
//       it.print(700, 100, id(segoeui_48), COLOR_OFF, TextAlign::TOP_RIGHT, "Offline");
//     }
     
// }