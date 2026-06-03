#include "wall_clock.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>

#include "resources.h"

static SDL_Point measure_text(TTF_Font *font, const char *text) {
  auto surface = TTF_RenderText_Solid(font, text, std::strlen(text),
                                      SDL_Color{0, 0, 0, 255});
  if (!surface) {
    throw std::runtime_error("TTF_RenderText_Solid");
  }
  SDL_Point size{surface->w, surface->h};
  SDL_DestroySurface(surface);
  return size;
}

void play_audio(void *userdata, SDL_AudioStream *stream, int additional_amount,
                int total_amount) {
  ((wall_clock *)userdata)->play_chimes(total_amount);
}

wall_clock::wall_clock(const std::string &help_path)
    : help_path_{help_path}, wnd_{nullptr}, renderer_{nullptr},
      font_source_{nullptr}, font_big_{nullptr}, font_medium_{nullptr},
      font_small_{nullptr}, width_{0}, height_{0}, digit_width_{0},
      colon_width_{0}, ampm_width_{0}, time_width_{0}, lines_height_{0},
      frame_time_{}, timer_base_{}, now_{0}, tense_{0}, pitch_{0}, volume_{0},
      display_{0}, text_color_{0, 0, 0, 0}, background_{0, 0, 0, 0},
      hide_cursor_{true}, fullscreen_{true}, dim_{true}, whisper_{true},
      has_chimes_{false}, has_alarms_{false}, has_sound_info_{true}, weekday_{},
      date_{}, time_24_{true}, seconds_{true}, pad_hour_{true},
      pad_minute_{true}, pad_second_{true}, pad_year_{true}, pad_month_{true},
      pad_day_{true}, timer_interval_{0}, next_alarm_{std::size_t(-1)},
      audio_stream_{nullptr}, texture_second_{nullptr}, size_second_{0, 0},
      texture_ampm_{nullptr}, size_ampm_{0, 0}, texture_time_{nullptr},
      size_time_{0, 0}, texture_weekday_{nullptr}, size_weekday_{0, 0},
      texture_date_{nullptr}, size_date_{0, 0}, texture_options_{nullptr},
      size_options_{0, 0}, total_height_{0} {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    throw std::runtime_error("SDL_INIT");
  }
  if (TTF_Init() < 0) {
    throw std::runtime_error("TTF_Init");
  }
  SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
  wnd_ = SDL_CreateWindow("Wall Clock", 800, 600, SDL_WINDOW_RESIZABLE);
  if (!wnd_) {
    throw std::runtime_error("SDL_create_window");
  }
  renderer_ = SDL_CreateRenderer(wnd_, nullptr);
  if (!renderer_) {
    throw std::runtime_error("SDL_CreateRenderer");
  }
  font_source_ = SDL_IOFromConstMem(Font_ttf, Font_ttf_size);
  if (!font_source_) {
    throw std::runtime_error("SDL_IOFromConstMem");
  }
  std::vector<int> sequence(13);
  std::iota(sequence.begin(), sequence.end(), 0);
  chimes_.insert(chimes_.end(), sequence.begin(), sequence.end());
  create_audio();
  set_config_handlers();
  set_display();
}

wall_clock::~wall_clock() {
  SDL_DestroyAudioStream(audio_stream_);
  TTF_CloseFont(font_big_);
  TTF_CloseFont(font_medium_);
  TTF_CloseFont(font_small_);
  TTF_Quit();
  SDL_CloseIO(font_source_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(wnd_);
  SDL_Quit();
}

void wall_clock::set_display() {
  int total = 0;
  auto dispalys = SDL_GetDisplays(&total);
  if (display_ != -1 && display_ < total &&
      dispalys[display_] != SDL_GetDisplayForWindow(wnd_)) {
    if ((SDL_GetWindowFlags(wnd_) & SDL_WINDOW_FULLSCREEN) ==
        SDL_WINDOW_FULLSCREEN) {
      if (!SDL_SetWindowFullscreen(wnd_, 0)) {
        throw std::runtime_error("SDL_SetWindowFullscreen(false)");
      }
    }
    SDL_Rect current_frame;
    if (!SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(wnd_),
                                    &current_frame)) {
      throw std::runtime_error("SDL_GetDisplayUsableBounds(current)");
    }
    SDL_Rect current_window;
    SDL_GetWindowPosition(wnd_, &current_window.x, &current_window.y);
    SDL_GetWindowSize(wnd_, &current_window.w, &current_window.h);
    SDL_Rect new_frame;
    if (!SDL_GetDisplayUsableBounds(dispalys[display_], &new_frame)) {
      throw std::runtime_error("SDL_GetDisplayUsableBounds(new)");
    }
    SDL_Rect new_window{
        new_frame.x + (current_window.x - current_frame.x) * new_frame.w /
                          current_frame.w,
        new_frame.y + (current_window.y - current_frame.y) * new_frame.h /
                          current_frame.h,
        current_window.w * new_frame.w / current_frame.w,
        current_window.h * new_frame.h / current_frame.h,
    };
    SDL_SetWindowPosition(wnd_, new_window.x, new_window.y);
    SDL_SetWindowSize(wnd_, new_window.w, new_window.h);
  }
  SDL_free(dispalys);
  set_window();
}

void wall_clock::set_window() {
  if (fullscreen_) {
    if ((SDL_GetWindowFlags(wnd_) & SDL_WINDOW_FULLSCREEN) !=
        SDL_WINDOW_FULLSCREEN) {
      if (!SDL_SetWindowFullscreen(wnd_, SDL_WINDOW_FULLSCREEN)) {
        throw std::runtime_error("SDL_SetWindowFullscreen(true)");
      }
    }
  } else {
    if ((SDL_GetWindowFlags(wnd_) & SDL_WINDOW_FULLSCREEN) ==
        SDL_WINDOW_FULLSCREEN) {
      if (!SDL_SetWindowFullscreen(wnd_, 0)) {
        throw std::runtime_error("SDL_SetWindowFullscreen(false)");
      }
    }
  }
  SDL_GetWindowSizeInPixels(wnd_, &width_, &height_);
  if (!SDL_SetRenderLogicalPresentation(renderer_, width_, height_,
                                        SDL_LOGICAL_PRESENTATION_STRETCH)) {
    throw std::runtime_error("SDL_SetRenderLogicalPresentation");
  }
  set_fonts();
}

void wall_clock::set_fonts() {
  lines_height_ = calculate_lines_height();
  reset_big_font();
  auto text_width = std::max(digit_width_ * 6 + colon_width_ * 2, width_);
  SDL_SeekIO(font_source_, 0, SDL_IO_SEEK_SET);
  TTF_CloseFont(font_medium_);
  font_medium_ =
      TTF_OpenFontIO(font_source_, false,
                     width_ * height_ * 2.0f / lines_height_ / text_width);
  if (!font_medium_) {
    throw std::runtime_error("TTF_OpenFont");
  }
  SDL_SeekIO(font_source_, 0, SDL_IO_SEEK_SET);
  TTF_CloseFont(font_small_);
  font_small_ =
      TTF_OpenFontIO(font_source_, false,
                     width_ * height_ * 1.0f / lines_height_ / text_width);
  if (!font_small_) {
    throw std::runtime_error("TTF_OpenFont");
  }
  int space_width, a_width, p_width, m_width;
  if (TTF_GetStringSize(font_medium_, " ", 1, &space_width, nullptr) < 0 ||
      TTF_GetStringSize(font_medium_, "A", 1, &a_width, nullptr) < 0 ||
      TTF_GetStringSize(font_medium_, "P", 1, &p_width, nullptr) < 0 ||
      TTF_GetStringSize(font_medium_, "M", 1, &m_width, nullptr) < 0) {
    throw std::runtime_error("TTF_GetStringSize");
  }
  ampm_width_ = space_width + std::max(a_width, p_width) + m_width;
  set_big_font();
}

void wall_clock::reset_big_font() {
  SDL_SeekIO(font_source_, 0, SDL_IO_SEEK_SET);
  TTF_CloseFont(font_big_);
  font_big_ =
      TTF_OpenFontIO(font_source_, false, height_ * 4.0f / lines_height_);
  if (!font_big_) {
    throw std::runtime_error("TTF_OpenFont");
  }
  if (TTF_GetStringSize(font_big_, "0", 1, &digit_width_, nullptr) < 0 ||
      TTF_GetStringSize(font_big_, ":", 1, &colon_width_, nullptr) < 0) {
    throw std::runtime_error("TTF_GetStringSize");
  }
  time_width_ = calculate_time_width();
}

void wall_clock::set_big_font() {
  SDL_SeekIO(font_source_, 0, SDL_IO_SEEK_SET);
  TTF_CloseFont(font_big_);
  font_big_ =
      TTF_OpenFontIO(font_source_, false,
                     (width_ - ampm_width_) * height_ * 4.0f / lines_height_ /
                         (std::max(time_width_, width_) - ampm_width_));
  if (!font_big_) {
    throw std::runtime_error("TTF_OpenFont");
  }
  if (TTF_GetStringSize(font_big_, "0", 1, &digit_width_, nullptr) < 0 ||
      TTF_GetStringSize(font_big_, ":", 1, &colon_width_, nullptr) < 0) {
    throw std::runtime_error("TTF_GetStringSize");
  }
  time_width_ = calculate_time_width();
}

void wall_clock::create_audio() {
  SDL_AudioSpec spec;
  SDL_zero(spec);
  spec.freq = SEGMENT_COUNT * SAMPLE_COUNT;
  spec.format = SDL_AUDIO_F32;
  spec.channels = 1;
  audio_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                            &spec, play_audio, this);
  if (!audio_stream_) {
    throw std::runtime_error("SDL_create_audio_stream");
  }
}

void wall_clock::set_config_handlers() {
  config_handlers_ = {
      {"alarm",
       [&](std::istream &is) {
         char colon;
         int hour, minute;
         if (is >> hour >> colon >> minute) {
           if (colon == ':' && hour < 24 && hour >= 0 && minute < 60 &&
               minute >= 0) {
             std::vector<std::size_t> alarm_weekdays;
             std::string weekday;
             bool inactive = false;
             while (is >> weekday) {
               std::transform<std::string::iterator, std::string::iterator,
                              int (*)(int)>(weekday.begin(), weekday.end(),
                                            weekday.begin(), std::toupper);
               if (weekday == "WEEKDAYS") {
                 for (const auto wd : {1, 2, 3, 4, 5}) {
                   alarm_weekdays.push_back(wd);
                 }
               } else if (weekday == "WEEKEND") {
                 for (const auto wd : {0, 6}) {
                   alarm_weekdays.push_back(wd);
                 }
               } else if (weekday == "NEVER") {
                 alarm_weekdays.clear();
                 inactive = true;
                 break;
               } else {
                 auto day = std::find(weekdays_full_.begin(),
                                      weekdays_full_.end(), weekday);
                 if (day != weekdays_full_.end()) {
                   alarm_weekdays.push_back(day - weekdays_full_.begin());
                 }
               }
             }
             if (alarm_weekdays.empty() && !inactive) {
               for (auto it = weekdays_full_.begin();
                    it != weekdays_full_.end(); ++it) {
                 alarm_weekdays.push_back(it - weekdays_full_.begin());
               }
             }
             for (const auto &alarm_weekday : alarm_weekdays) {
               alarms_.insert((alarm_weekday * 24 + hour) * 60 + minute);
             }
           }
         }
       }},
      {"volume",
       [&](std::istream &is) {
         int volume;
         if (is >> volume && volume >= 0 && volume <= 100) {
           volume_ = volume;
         }
       }},
      {"display",
       [&](std::istream &is) {
         int display;
         if (is >> display && display >= 0) {
           display_ = display - 1;
         }
       }},
      {"color",
       [&](std::istream &is) {
         int r, g, b;
         if (is >> r >> g >> b) {
           text_color_.r = r;
           text_color_.g = g;
           text_color_.b = b;
         }
       }},
      {"background",
       [&](std::istream &is) {
         int r, g, b;
         if (is >> r >> g >> b) {
           background_.r = r;
           background_.g = g;
           background_.b = b;
         }
       }},
      {"hide-cursor",
       [&](std::istream &is) {
         std::string hide_cursor;
         if (is >> hide_cursor) {
           if (hide_cursor == "true") {
             hide_cursor_ = true;
           } else if (hide_cursor == "false") {
             hide_cursor_ = false;
           }
         }
       }},
      {"fullscreen",
       [&](std::istream &is) {
         std::string fullscreen;
         if (is >> fullscreen) {
           if (fullscreen == "true") {
             fullscreen_ = true;
           } else if (fullscreen == "false") {
             fullscreen_ = false;
           }
         }
       }},
      {"dim",
       [&](std::istream &is) {
         std::string dim;
         if (is >> dim) {
           if (dim == "true") {
             dim_ = true;
           } else if (dim == "false") {
             dim_ = false;
           }
         }
       }},
      {"whisper",
       [&](std::istream &is) {
         std::string whisper;
         if (is >> whisper) {
           if (whisper == "true") {
             whisper_ = true;
           } else if (whisper == "false") {
             whisper_ = false;
           }
         }
       }},
      {"chimes",
       [&](std::istream &is) {
         std::string chimes;
         if (is >> chimes) {
           if (chimes == "true") {
             has_chimes_ = true;
           } else if (chimes == "false") {
             has_chimes_ = false;
           }
         }
       }},
      {"alarms",
       [&](std::istream &is) {
         std::string alarms;
         if (is >> alarms) {
           if (alarms == "true") {
             has_alarms_ = true;
           } else if (alarms == "false") {
             has_alarms_ = false;
           }
         }
       }},
      {"sound-info",
       [&](std::istream &is) {
         std::string sound_info;
         if (is >> sound_info) {
           if (sound_info == "true") {
             has_sound_info_ = true;
           } else if (sound_info == "false") {
             has_sound_info_ = false;
           }
         }
       }},
      {"weekday",
       [&](std::istream &is) {
         std::string weekday;
         if (is >> weekday) {
           if (weekday.size() <= 10) {
             weekday_ = weekday;
           }
         }
       }},
      {"date",
       [&](std::istream &is) {
         std::string date;
         if (is >> date) {
           if (date.size() <= 10) {
             date_ = date;
           }
         }
       }},
      {"24-hour",
       [&](std::istream &is) {
         std::string time_24;
         if (is >> time_24) {
           if (time_24 == "true") {
             time_24_ = true;
           } else if (time_24 == "false") {
             time_24_ = false;
           }
         }
       }},
      {"seconds",
       [&](std::istream &is) {
         std::string seconds;
         if (is >> seconds) {
           if (seconds == "true") {
             seconds_ = true;
           } else if (seconds == "false") {
             seconds_ = false;
           }
         }
       }},
      {"pad-hour",
       [&](std::istream &is) {
         std::string pad_hour;
         if (is >> pad_hour) {
           if (pad_hour == "true") {
             pad_hour_ = true;
           } else if (pad_hour == "false") {
             pad_hour_ = false;
           }
         }
       }},
      {"pad-minute",
       [&](std::istream &is) {
         std::string pad_minute;
         if (is >> pad_minute) {
           if (pad_minute == "true") {
             pad_minute_ = true;
           } else if (pad_minute == "false") {
             pad_minute_ = false;
           }
         }
       }},
      {"pad-second",
       [&](std::istream &is) {
         std::string pad_second;
         if (is >> pad_second) {
           if (pad_second == "true") {
             pad_second_ = true;
           } else if (pad_second == "false") {
             pad_second_ = false;
           }
         }
       }},
      {"pad-year",
       [&](std::istream &is) {
         std::string pad_year;
         if (is >> pad_year) {
           if (pad_year == "true") {
             pad_year_ = true;
           } else if (pad_year == "false") {
             pad_year_ = false;
           }
         }
       }},
      {"pad-month",
       [&](std::istream &is) {
         std::string pad_month;
         if (is >> pad_month) {
           if (pad_month == "true") {
             pad_month_ = true;
           } else if (pad_month == "false") {
             pad_month_ = false;
           }
         }
       }},
      {"pad-day",
       [&](std::istream &is) {
         std::string pad_day;
         if (is >> pad_day) {
           if (pad_day == "true") {
             pad_day_ = true;
           } else if (pad_day == "false") {
             pad_day_ = false;
           }
         }
       }},
      {"timer-interval",
       [&](std::istream &is) {
         int timer_interval;
         if (is >> timer_interval) {
           if (timer_interval > 0) {
             timer_interval_ = timer_interval;
           }
         }
       }},
  };
}

int wall_clock::calculate_time_width() {
  return digit_width_ * 4 + colon_width_ +
         (seconds_ ? digit_width_ * 2 + colon_width_ : 0) +
         (time_24_ ? 0 : ampm_width_);
}

int wall_clock::calculate_lines_height() {
  return (has_sound_info_ ? 1 : 0) + (date_ != "?" ? 2 : 0) +
         (weekday_ != "?" ? 2 : 0) + 4;
}

float wall_clock::get_volume() {
  return (volume_ / 100.0f) * (whisper_ ? tense_ : 1.0f) * 0.5f;
}

int wall_clock::chime_count(int hour) {
  return hour % 12 != 0 ? hour % 12 : 12;
}

const char *wall_clock::ampm(int hour) { return hour < 12 ? " AM" : " PM"; }

void wall_clock::run() {
  for (;;) {
    auto now = std::chrono::system_clock::now();
    auto tmp =
        now.time_since_epoch().count() % std::chrono::system_clock::period::den;
    now = now - std::chrono::system_clock::duration{tmp};
    if (now != frame_time_) {
      frame_time_ = now;
      tick();
    } else {
      SDL_Event event;
      if (SDL_WaitEventTimeout(
              &event, 1000 - (tmp * 1000 /
                              std::chrono::system_clock::period::den)) == 1) {
        if (handle_event(&event) < 0) {
          break;
        }
      }
    }
  }
}

int wall_clock::handle_event(SDL_Event *event) {
  int iResult = 0;
  switch (event->type) {
  case SDL_EVENT_QUIT:
    iResult = -1;
    break;
  case SDL_EVENT_KEY_UP:
    switch (event->key.scancode) {
    case SDL_SCANCODE_F1:
      if (SDL_OpenURL(help_path_.c_str()) != 0) {
        throw std::runtime_error("SDL_OpenURL(help)");
      }
      break;
    case SDL_SCANCODE_ESCAPE:
      iResult = -1;
      break;
    case SDL_SCANCODE_SPACE:
      silent();
      break;
    case SDL_SCANCODE_R:
      test_bell();
      break;
    case SDL_SCANCODE_RETURN:
      read_config();
      redraw(false);
      break;
    case SDL_SCANCODE_0:
      start_timer(0);
      break;
    case SDL_SCANCODE_1:
      start_timer(1);
      break;
    case SDL_SCANCODE_2:
      start_timer(2);
      break;
    case SDL_SCANCODE_3:
      start_timer(3);
      break;
    case SDL_SCANCODE_4:
      start_timer(4);
      break;
    case SDL_SCANCODE_5:
      start_timer(5);
      break;
    case SDL_SCANCODE_6:
      start_timer(6);
      break;
    case SDL_SCANCODE_7:
      start_timer(7);
      break;
    case SDL_SCANCODE_8:
      start_timer(8);
      break;
    case SDL_SCANCODE_9:
      start_timer(9);
      break;
    case SDL_SCANCODE_MINUS:
      stop_timer();
      break;
    }
    break;
  case SDL_EVENT_MOUSE_BUTTON_UP:
    iResult = -1;
    break;
  case SDL_EVENT_FINGER_UP:
    iResult = -1;
    break;
  case SDL_EVENT_WINDOW_RESIZED:
    set_window();
    redraw(false);
    break;
  }
  return iResult;
}

void wall_clock::read_config() {
  volume_ = 100;
  display_ = -1;
  text_color_ = {255, 255, 255, 255};
  background_ = {0, 0, 0, 255};
  hide_cursor_ = true;
  fullscreen_ = true;
  dim_ = true;
  whisper_ = true;
  has_chimes_ = false;
  has_alarms_ = false;
  has_sound_info_ = true;
  weekday_ = "%A";
  date_ = "%m/%d/%Y";
  time_24_ = true;
  seconds_ = true;
  pad_hour_ = true;
  pad_minute_ = true;
  pad_second_ = true;
  pad_year_ = true;
  pad_month_ = true;
  pad_day_ = true;
  timer_interval_ = 10;
  alarms_.clear();
  next_alarm_ = std::size_t(-1);
  const char *home_directory = getenv(HOME);
  if (home_directory) {
    std::string conf_path{home_directory};
    conf_path += "/.clock.conf";
    std::ifstream config_file(conf_path.c_str());
    std::string config;
    while (std::getline(config_file, config)) {
      std::istringstream pair_stream{config};
      std::string key;
      pair_stream >> key;
      auto handler = config_handlers_.find(key);
      if (handler != config_handlers_.end()) {
        handler->second(pair_stream);
      }
    }
    bool will_alarm = false;
    std::size_t alarm_time =
        (now_.tm_wday * 24 + now_.tm_hour) * 60 + now_.tm_min;
    auto alarm = alarms_.lower_bound(alarm_time);
    if (alarm != alarms_.end() && *alarm == alarm_time) {
      if (has_alarms_) {
        bell_alarm();
        will_alarm = true;
      }
      ++alarm;
    }
    if (has_chimes_) {
      if (now_.tm_min == 0 && !will_alarm) {
        bell_chime();
      }
    }
    if (alarm == alarms_.end()) {
      alarm = alarms_.begin();
    }
    if (alarm != alarms_.end()) {
      next_alarm_ = *alarm;
    }
  }
  if (hide_cursor_) {
    SDL_HideCursor();
  } else {
    SDL_ShowCursor();
  }
  if (display_ >= 0 && SDL_GetDisplayForWindow(wnd_) != display_) {
    set_display();
  } else if (((SDL_GetWindowFlags(wnd_) & SDL_WINDOW_FULLSCREEN) ==
              SDL_WINDOW_FULLSCREEN) != fullscreen_) {
    set_window();
  } else if (lines_height_ != calculate_lines_height()) {
    set_fonts();
  } else if (time_width_ != calculate_time_width()) {
    reset_big_font();
    set_big_font();
  }
}

void wall_clock::tick() {
  static std::time_t tPre = std::chrono::system_clock::to_time_t(
      frame_time_ - std::chrono::minutes(1));
  std::time_t t = std::chrono::system_clock::to_time_t(frame_time_);
  if (tPre != t) {
    now_ = *std::localtime(&t);
    auto pre = *std::localtime(&tPre);
    if (pre.tm_min != now_.tm_min) {
      tense_ = std::max(0, 8 * 60 - std::abs(now_.tm_hour * 60 + now_.tm_min -
                                             14 * 60)) /
                   static_cast<float>(8 * 60) * 0.85f +
               0.15f;
      pitch_ = 12 - std::abs(now_.tm_hour - 12);
      read_config();
      redraw(false);
    } else if (timer_base_.time_since_epoch().count() != 0) {
      redraw(false);
    } else if (seconds_) {
      redraw(true);
    }
    tPre = t;
  }
}

void wall_clock::redraw(const bool second_only) {
  text_color_.a = 255 * (dim_ ? tense_ : 1.0);
  background_.a = 255 * (dim_ ? tense_ : 1.0);
  if (seconds_) {
    std::stringstream sSecond;
    sSecond << ":" << std::setfill('0') << std::setw(pad_second_ ? 2 : 0)
            << now_.tm_sec;
    draw_text(texture_second_, size_second_, sSecond.str(), font_big_,
              text_color_);
  }
  if (!second_only) {
    total_height_ = 0;

    if (!time_24_) {
      auto ap = ampm(now_.tm_hour);
      draw_text(texture_ampm_, size_ampm_, ap, font_medium_, text_color_);
    }

    std::stringstream sTime;
    sTime << std::setfill('0') << std::setw(pad_hour_ ? 2 : 0)
          << (time_24_ ? now_.tm_hour : chime_count(now_.tm_hour)) << ":"
          << std::setw(pad_minute_ ? 2 : 0) << now_.tm_min;
    draw_text(texture_time_, size_time_, sTime.str(), font_big_, text_color_);
    total_height_ += size_time_.y;

    if (weekday_ != "?") {
      std::stringstream sWeekday;
      if (timer_base_.time_since_epoch().count() != 0) {
        int seconds = (frame_time_ - timer_base_) / std::chrono::seconds(1);
        sWeekday << std::setfill('0') << (seconds < 0 ? "-" : " ")
                 << std::setw(pad_minute_ ? 2 : 0) << std::abs(seconds) / 60
                 << ":" << std::setw(pad_second_ ? 2 : 0)
                 << std::abs(seconds) % 60 << " ";
        if (seconds == -2) {
          bell(3, 12, 1.0f);
        } else if (seconds >= 0 && seconds % timer_interval_ == 0) {
          bell(1, std::min(12, 2 + seconds / timer_interval_), 1.0f);
        }
      } else {
        bool ctrl = false;
        for (const auto &c : weekday_) {
          if (ctrl) {
            switch (c) {
            case 'A':
              sWeekday << weekdays_full_[now_.tm_wday];
              break;
            case 'a':
              sWeekday << weekdays_abbreviated_[now_.tm_wday];
              break;
            case 'w':
              sWeekday << now_.tm_wday;
              break;
            case 'u':
              sWeekday << (now_.tm_wday > 0 ? now_.tm_wday : 7);
              break;
            }
            ctrl = false;
          } else if (c == '%') {
            ctrl = true;
          } else {
            sWeekday << c;
          }
        }
      }
      draw_text(texture_weekday_, size_weekday_, sWeekday.str(), font_medium_,
                text_color_);
      total_height_ += size_weekday_.y;
    }

    if (date_ != "?") {
      std::stringstream sDate;
      sDate << std::setfill('0');
      bool ctrl = false;
      for (const auto &c : date_) {
        if (ctrl) {
          switch (c) {
          case 'm':
            sDate << std::setw(pad_month_ ? 2 : 0) << now_.tm_mon + 1;
            break;
          case 'b':
            sDate << months_[now_.tm_mon];
            break;
          case 'd':
            sDate << std::setw(pad_day_ ? 2 : 0) << now_.tm_mday;
            break;
          case 'Y':
            sDate << now_.tm_year + 1900;
            break;
          case 'y':
            sDate << std::setw(pad_year_ ? 2 : 0) << now_.tm_year % 100;
            break;
          }
          ctrl = false;
        } else if (c == '%') {
          ctrl = true;
        } else {
          sDate << c;
        }
      }
      auto date = sDate.str();
      draw_text(texture_date_, size_date_, date, font_medium_, text_color_);
      total_height_ += size_date_.y;
    }

    if (has_sound_info_) {
      std::stringstream sInfo;
      sInfo << "\x5:" << (has_chimes_ ? '\x7' : '\x8') << "  "
            << "\x6:" << (has_alarms_ ? '\x7' : '\x8') << " ";
      if (next_alarm_ != std::size_t(-1)) {
        auto day = next_alarm_ / (60 * 24);
        auto hour = next_alarm_ / 60 % 24;
        auto minute = next_alarm_ % 60;
        sInfo << weekdays_abbreviated_[day] << " " << std::setfill('0')
              << std::setw(pad_hour_ ? 2 : 0)
              << (time_24_ ? hour : chime_count(hour)) << ':'
              << std::setw(pad_minute_ ? 2 : 0) << minute
              << (time_24_ ? "" : ampm(hour));
      }
      draw_text(texture_options_, size_options_, sInfo.str(), font_small_,
                text_color_);
      total_height_ += size_options_.y;
    }
  }
  if (!SDL_SetRenderDrawColor(renderer_, background_.r, background_.g,
                              background_.b, background_.a) ||
      !SDL_RenderClear(renderer_)) {
    throw std::runtime_error("Clear Background");
  }
  auto space = (height_ - total_height_) /
               (2 + (weekday_ != "?" ? 1 : 0) + (date_ != "?" ? 1 : 0) +
                (has_sound_info_ ? 1 : 0));
  float x;
  float y = space;

  x = (width_ - size_time_.x - (seconds_ ? size_second_.x : 0) -
       (time_24_ ? 0 : size_ampm_.x)) /
      2;
  render_texture(texture_time_, x, y, size_time_);
  x += size_time_.x;
  if (seconds_) {
    render_texture(texture_second_, x, y, size_second_);
    x += size_second_.x;
  }
  if (!time_24_) {
    render_texture(texture_ampm_, x, y + (size_time_.y - size_ampm_.y) / 2,
                   size_ampm_);
    x += size_ampm_.x;
  }
  y += size_time_.y + space;
  if (weekday_ != "?") {
    x = (width_ - size_weekday_.x) / 2;
    render_texture(texture_weekday_, x, y, size_weekday_);
    y += size_weekday_.y + space;
  }
  if (date_ != "?") {
    x = (width_ - size_date_.x) / 2;
    render_texture(texture_date_, x, y, size_date_);
    y += size_date_.y + space;
  }
  if (has_sound_info_) {
    x = (width_ - size_options_.x) / 2;
    render_texture(texture_options_, x, y, size_options_);
    y += size_date_.y + space;
  }
  SDL_RenderPresent(renderer_);
}

void wall_clock::draw_text(SDL_Texture *&texture, SDL_FPoint &size,
                           const std::string &text, TTF_Font *font,
                           const SDL_Color &color) {
  auto surface = TTF_RenderText_Solid(font, text.c_str(), text.size(), color);
  if (!surface) {
    throw std::runtime_error("TTF_RenderText_Solid");
  }
  if (texture) {
    SDL_DestroyTexture(texture);
  }
  texture = SDL_CreateTextureFromSurface(renderer_, surface);
  if (!texture) {
    throw std::runtime_error("SDL_CreateTextureFromSurface");
  }
  if (!SDL_GetTextureSize(texture, &size.x, &size.y)) {
    throw std::runtime_error("SDL_GetTextureSize");
  }
  SDL_DestroySurface(surface);
}

void wall_clock::render_texture(SDL_Texture *texture, float x, float y,
                                const SDL_FPoint &size) {
  SDL_FRect dest{x, y, size.x, size.y};
  if (!SDL_RenderTexture(renderer_, texture, nullptr, &dest)) {
    throw std::runtime_error("SDL_RenderTexture");
  }
}

void wall_clock::start_timer(int delay) {
  timer_base_ = frame_time_ + std::chrono::seconds(delay * timer_interval_);
}

void wall_clock::stop_timer() {
  timer_base_ = {};
  redraw(false);
}

void wall_clock::bell_alarm() {
  SDL_LockAudioStream(audio_stream_);
  if (strikes_.empty()) {
    for (int i = 0; i < 13; ++i) {
      strikes_.push_back(
          {-int((i * 4.0f + 0.0f) * SEGMENT_COUNT) * SAMPLE_COUNT,
           get_volume() * (i + 1) / 13.0f, i});
      strikes_.push_back(
          {-int((i * 4.0f + 1.0f) * SEGMENT_COUNT) * SAMPLE_COUNT,
           get_volume() * (i + 1) / 26.0f, i});
    }
  }
  SDL_UnlockAudioStream(audio_stream_);
  SDL_ResumeAudioStreamDevice(audio_stream_);
}

void wall_clock::bell_chime() {
  SDL_LockAudioStream(audio_stream_);
  if (strikes_.empty()) {
    for (int i = 0; i < chime_count(now_.tm_hour); ++i) {
      strikes_.push_back(
          {-int((i * 1.5f + 0.0f) * SEGMENT_COUNT) * SAMPLE_COUNT, get_volume(),
           pitch_});
    }
  }
  SDL_UnlockAudioStream(audio_stream_);
  SDL_ResumeAudioStreamDevice(audio_stream_);
}

void wall_clock::bell(int count, int pitch, float delay) {
  SDL_LockAudioStream(audio_stream_);
  if (strikes_.empty()) {
    for (int i = 0; i < count; ++i) {
      strikes_.push_back({-int((i * delay) * SEGMENT_COUNT) * SAMPLE_COUNT,
                          get_volume(), pitch});
    }
  }
  SDL_UnlockAudioStream(audio_stream_);
  SDL_ResumeAudioStreamDevice(audio_stream_);
}

void wall_clock::silent() {
  SDL_LockAudioStream(audio_stream_);
  strikes_.remove_if([](auto &strike) { return strike.pos <= 0; });
  SDL_UnlockAudioStream(audio_stream_);
}

void wall_clock::test_bell() { bell(2, pitch_, 2.0f); }

void wall_clock::play_chimes(int length) {
  audio_buffer_.resize(length / sizeof(float));
  SDL_memset(audio_buffer_.data(), 0, length);
  SDL_LockAudioStream(audio_stream_);
  if (strikes_.empty())
    SDL_PauseAudioStreamDevice(audio_stream_);
  else {
    for (auto it = strikes_.begin(); it != strikes_.end();) {
      if (chimes_[it->pitch].play(it->volume / 1.7f, it->pos,
                                  audio_buffer_.data(), audio_buffer_.size())) {
        ++it;
      } else {
        it = strikes_.erase(it);
      }
    }
  }
  SDL_UnlockAudioStream(audio_stream_);
  SDL_PutAudioStreamData(audio_stream_, audio_buffer_.data(),
                         audio_buffer_.size() * sizeof(float));
}
