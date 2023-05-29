#pragma once
#include "esphome.h"

constexpr const char *TextNA = "N/A";
constexpr const char *TextOpen = "open";
constexpr const char *TextClosed = "closed";

constexpr const char *IconAlert = "\U000F0026";
constexpr const char *IconCooktop = "\U000F065A";
constexpr const char *IconFridge = "\U000F0290";
constexpr const char *IconOven = "\U000F0CD3";
constexpr const char *IconWindow = "\U000F05B1";

std::map<std::string, std::string> weather_icon_map
  {
    {"cloudy", "\U000F0590"},
    {"cloudy-alert", "\U000F0F2F"},
    {"cloudy-arrow-right", "\U000F0E6E"},
    {"fog", "\U000F0591"},
    {"hail", "\U000F0592"},
    {"hazy", "\U000F0F30"},
    {"hurricane", "\U000F0898"},
    {"lightning", "\U000F0593"},
    {"lightning-rainy", "\U000F067E"},
    {"night", "\U000F0594"},
    {"night-partly-cloudy", "\U000F0F31"},
    {"partlycloudy", "\U000F0595"},
    {"partly-lightning", "\U000F0F32"},
    {"partly-rainy", "\U000F0F33"},
    {"partly-snowy", "\U000F0F34"},
    {"partly-snowy-rainy", "\U000F0F35"},
    {"pouring", "\U000F0596"},
    {"rainy", "\U000F0597"},
    {"snowy", "\U000F0598"},
    {"snowy-heavy", "\U000F0F36"},
    {"snowy-rainy", "\U000F067F"},
    {"sunny", "\U000F0599"},
    {"sunny-alert", "\U000F0F37"},
    {"sunny-off", "\U000F14E4"},
    {"sunset", "\U000F059A"},
    {"sunset-down", "\U000F059B"},
    {"sunset-up", "\U000F059C"},
    {"tornado", "\U000F0F38"},
    {"windy", "\U000F059D"},
    {"windy-variant", "\U000F059E"},
  };

struct SensorValue {
public:
  std::string text;
  std::string icon;
  bool is_ok = false;
public:
  SensorValue(std::string text, std::string icon, bool is_ok)
    : text{std::move(text)}, icon{std::move(icon)}, is_ok{is_ok} {}
};

class SensorModel {
public:
  SensorModel() {
    add_open_closed(id(gallery_window), IconWindow);
    add_open_closed(id(guestroom_windows), IconWindow);
    add_open_closed(id(bedroom_windows), IconWindow);
    add_open_closed(id(bathroom_window), IconWindow);
    add_open_closed(id(dining_room_windows), IconWindow);
    add_open_closed(id(living_room_windows), IconWindow);
    add_open_closed(id(fridge_door), IconFridge);
    add_text_sensor(id(oven), IconOven);
    add_text_sensor(id(cooktop), IconCooktop);
    add_open_closed(id(test_bool), IconAlert);
  }

  bool are_all_ok() const {
    return are_all_ok_;
  }

  bool are_all_available() const {
    return are_all_available_;
  }

  const std::map<std::string, SensorValue> &get_sensors() const {
    return sensors_;
  }

private:
  void add(std::string name, std::string value, std::string icon, bool is_ok) {
    are_all_ok_ &= is_ok;
    sensors_.insert(
      std::make_pair(
        std::move(name),
        SensorValue(std::move(value), std::move(icon), is_ok)
      )
    );
  }

  void add_open_closed(binary_sensor::BinarySensor &sensor, std::string icon) {
    bool is_available = sensor.has_state();
    bool is_ok = is_available && !sensor.state;
    const char *text = is_available
      ? sensor.state ? TextOpen : TextClosed
      : TextNA;
    add(sensor.get_name(), text, std::move(icon), is_ok);
    are_all_available_ &= is_available;
  }
  
  void add_text_sensor(text_sensor::TextSensor &sensor, std::string icon) {
    bool is_available = sensor.has_state();
    bool is_ok = is_available && sensor.state == "off";
    std::string text = is_available ? sensor.state : TextNA;
    add(sensor.get_name(), std::move(text), std::move(icon), is_ok);
    are_all_available_ &= is_available;
  }

private:
  std::map<std::string, SensorValue> sensors_;
  bool are_all_ok_ = true;
  bool are_all_available_ = true;
};


struct WeatherModel {
  std::string time;
  std::string icon;
  std::string temperature;

  WeatherModel(
    std::string time,
    const std::string& condition,
    std::string temperature
  ) : time{std::move(time)},
      temperature{std::move(temperature)} {
    auto it = weather_icon_map.find(condition);
    if (it != weather_icon_map.end()) {
      icon = it->second;
    }
  }

  WeatherModel(
    text_sensor::TextSensor& time,
    text_sensor::TextSensor& condition,
    text_sensor::TextSensor& temperature
  ) : WeatherModel{time.state, condition.state, temperature.state} {}
};

struct BlockModel {
  std::string icon;
  std::string text;
};

void draw_date_time_big(LayoutContext &layout) {
  char buff[64] = "";
  auto now = id(esptime).now();

  layout.skip_vertical(32);
  now.strftime(buff, sizeof(buff), "%A %d %B");
  layout.print(buff, TextAlign::TOP_CENTER);
  layout.new_line();


  now.strftime(buff, sizeof(buff), "%I:%M");
  char *time = (*buff == '0') ? (buff + 1) : buff;
  layout.print(time, TextAlign::TOP_CENTER, &id(time_font));
  layout.new_line();

  layout.skip_vertical(24);
}

void draw_weather_small(LayoutContext &parent, const WeatherModel &model) {
  if (model.icon.empty() || model.time.empty()) {
    return;
  }
  auto layout = parent.reserve_left(135);
  layout.print(model.time.c_str(), TextAlign::TOP_CENTER);
  layout.new_line();
  layout.print(model.icon.c_str(), TextAlign::TOP_CENTER, &id(font_mdi_large));
  layout.new_line();
  layout.print(model.temperature.c_str(), TextAlign::TOP_CENTER);
  layout.new_line();
}

void draw_block(LayoutContext &layout, const BlockModel &block) {
  constexpr int BlockSize = 120;

  auto block_layout = layout.reserve_top(BlockSize);
  auto icon_layout = block_layout.reserve_left(BlockSize);

  block_layout.fill_rounded(COLOR_OFF, 16);

  icon_layout.print(
    block.icon.c_str(),
    TextAlign::CENTER,
    &id(font_mdi_large),
    COLOR_ON
  );

  block_layout.print(
    block.text.c_str(),
    TextAlign::CENTER_LEFT,
    COLOR_ON
  );

  layout.skip_vertical(12);
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
    if (model.are_all_available()) {
      if (model.are_all_ok()) {
        int y = rect_.y2() - id(totoro).get_height();
        buffer_.image(0, y, &id(totoro));
      } else {
        for (const auto& p : model.get_sensors()) {
          if (!p.second.is_ok) {
            BlockModel block;
            block.icon = p.second.icon;
            block.text = p.first + ' ' + p.second.text;
            draw_block(layout, block);
          }
        }
      }
    } else {
      BlockModel block;
      block.icon = IconAlert;
      block.text = "Data unavailable";
      draw_block(layout, block);
    }

    layout.skip_vertical(32);

    draw_weather_small(layout, {
      "Now",
      id(weather_now_condition).state,
      id(weather_now_temperature).state
    });
    draw_weather_small(layout, {
      id(weather_0_timestamp),
      id(weather_0_condition),
      id(weather_0_temperature)
    });
    draw_weather_small(layout, {
      id(weather_1_timestamp),
      id(weather_1_condition),
      id(weather_1_temperature)
    });
    draw_weather_small(layout, {
      id(weather_2_timestamp),
      id(weather_2_condition),
      id(weather_2_temperature)
    });
    draw_weather_small(layout, {
      id(weather_3_timestamp),
      id(weather_3_condition),
      id(weather_3_temperature)
    });
  }

private:
  display::DisplayBuffer &buffer_;
  display::Rect rect_;

  static constexpr int H_MARGIN = 12;
  static constexpr int LINE_SPACING = 4;

};