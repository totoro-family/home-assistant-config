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
    int line_spacing = 4,
    int span_spacing = 4
  ) 
    : buffer_{buffer},
      rect_{rect},
      line_spacing_{line_spacing},
      span_spacing_{span_spacing} {
    rect_.expand(-horizontal_margin, 0);
  }
 
  void print(const char *text, TextAlign align) {
    print(text, align, &id(default_font));
  }

  void print(const char *text, TextAlign align, Font *font) {

    int x1, y1, w, h;
    buffer_.get_text_bounds(
      0, 0,
      text,
      font,
      TextAlign::TOP_LEFT,
      &x1, &y1, &w, &h);

    buffer_.start_clipping(rect_);

    int x;
    if (align == TextAlign::TOP_RIGHT) {
      x = rect_.x2() - current_span_right_;
      current_span_left_ += w + span_spacing_;
    } else if (align == TextAlign::TOP_CENTER) {
      int avail_w = (rect_.w - current_span_left_ - current_span_right_);
      x = rect_.x + (avail_w / 2);
    } else {
      x = rect_.x + current_span_left_;
      current_span_left_ += w + span_spacing_;
    }

    buffer_.print(
      x,
      rect_.y,
      font,
      COLOR_OFF,
      align,
      text
    );

    buffer_.end_clipping();

    if (h > current_line_height_) {
      current_line_height_ = h;
    }
  }

  void new_line() {
    if (current_line_height_ > 0) {
      skip_vertical(current_line_height_ + line_spacing_);
    }
  }

  void skip_vertical(int height) {
      rect_.y += height;
      rect_.h -= height;
      current_line_height_ = 0;
      current_span_left_ = 0;
      current_span_right_ = 0;
  }

private:
  display::DisplayBuffer &buffer_;
  display::Rect rect_;
  int line_spacing_;
  int span_spacing_;
  int current_line_height_ = 0;
  int current_span_left_ = 0;
  int current_span_right_ = 0;

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

void draw_date_time_big(LayoutContext &layout) {
  char buff[64] = "";
  auto now = id(esptime).now();

  layout.skip_vertical(32);
  now.strftime(buff, sizeof(buff), "%A %d %B");
  layout.print(buff, TextAlign::TOP_CENTER);
  layout.new_line();

  now.strftime(buff, sizeof(buff), "%H:%M");
  layout.print(buff, TextAlign::TOP_CENTER, &id(time_font));
  layout.new_line();

  layout.skip_vertical(24);
}

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

    draw_date_time_big(layout);

    SensorModel model;
    if (model.is_all_ok()) {
      int y = rect_.y2() - id(totoro).get_height();
      buffer_.image(0, y, &id(totoro));
    } else {
      for (const auto& p : model.get_sensors()) {
        layout.print(p.first.c_str(), TextAlign::TOP_LEFT);
        layout.print(p.second.text.c_str(), TextAlign::TOP_RIGHT);
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