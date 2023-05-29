#pragma once
#include "esphome.h"

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
 
  LayoutContext reserve_left(int w) {
    display::Rect left{rect_};

    left.x = rect_.x + current_span_left_;
    left.w = w;

    current_span_left_ += w + span_spacing_;
    return LayoutContext{buffer_, left};
  }
 
  LayoutContext reserve_top(int h) {
    display::Rect rect{rect_};
    rect.h = h;
    skip_vertical(h);
    return LayoutContext{buffer_, rect};
  }

  LayoutContext reserve_bottom(int h) {
    if (h > rect_.h) {
        h = rect_.h;
    }

    display::Rect rect{rect_};
    rect.y = rect_.y2() - h;
    rect.h = h;
    rect_.h -= h;

    return LayoutContext{buffer_, rect};

  }
 
  int current_line_height() const {
    return current_line_height_;
  }

  int width() const {
    return rect_.w;
  }

  void fill(Color color) {
    buffer_.filled_rectangle(rect_.x, rect_.y, rect_.w, rect_.h, color);
  }

  void fill_rounded(Color fill, int radius) {
    buffer_.filled_rectangle(
      rect_.x + radius,
      rect_.y,
      rect_.w - (2 * radius),
      radius,
      fill
    );
    buffer_.filled_rectangle(
      rect_.x,
      rect_.y + radius,
      rect_.w,
      rect_.h - (2 * radius),
      fill
    );
    buffer_.filled_rectangle(
      rect_.x + radius,
      rect_.y2() - radius,
      rect_.w - (2 * radius),
      radius,
      fill
    );

    for (int i = 0; i < radius; i++) {
      for (int j = 0; j < radius; j++) {
        int x = i - radius;
        int y = j - radius;
        if (x*x + y*y <= radius*radius + 1) {
          buffer_.draw_pixel_at(
            rect_.x + i,
            rect_.y + j,
            fill
          );
          buffer_.draw_pixel_at(
            rect_.x2() - i - 1,
            rect_.y2() - j - 1,
            fill
          );
          buffer_.draw_pixel_at(
            rect_.x + i,
            rect_.y2() - j - 1,
            fill
          );
          buffer_.draw_pixel_at(
            rect_.x2() - i - 1,
            rect_.y + j,
            fill
          );
        }
      }
    }
  }

  void print(const char *text, Color color = COLOR_OFF) {
    print(text, TextAlign::TOP_LEFT, color);
  }

  void print(const char *text, Font *font, Color color = COLOR_OFF) {
    print(text, TextAlign::TOP_LEFT, font, color);
  }

  void print(const char *text, TextAlign align, Color color = COLOR_OFF) {
    print(text, align, &id(default_font), color);
  }

  void print(const char *text, TextAlign align, Font *font, Color color = COLOR_OFF) {

    int x1, y1, w, h;
    buffer_.get_text_bounds(
      0, 0,
      text,
      font,
      TextAlign::TOP_LEFT,
      &x1, &y1, &w, &h);

    buffer_.start_clipping(rect_);

    auto x_align = TextAlign(int(align) & 0x18);
    auto y_align = TextAlign(int(align) & 0x07);

    int x, y;

    if (x_align == TextAlign::RIGHT) {
      x = rect_.x2() - current_span_right_;
      current_span_left_ += w + span_spacing_;
    } else if (x_align == TextAlign::CENTER_HORIZONTAL) {
      int avail_w = (rect_.w - current_span_left_ - current_span_right_);
      x = rect_.x + (avail_w / 2);
    } else {
      x = rect_.x + current_span_left_;
      current_span_left_ += w + span_spacing_;
    }

    if (y_align == TextAlign::BOTTOM) {
      y = rect_.y2() - line_spacing_;
    } else if (y_align == TextAlign::CENTER_VERTICAL) {
      y = rect_.y + (rect_.h / 2) - line_spacing_;
    } else {
      y = rect_.y;
    }

    buffer_.print(
      x,
      y,
      font,
      color,
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
      skip_vertical(current_line_height_);
    }
  }

  void skip_vertical(int height) {
      height += line_spacing_;
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
