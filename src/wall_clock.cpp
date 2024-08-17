#include "wall_clock.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>
#include <string>

#include "resources.h"

std::vector<const char*> wall_clock::weekdays_ = {
    "SUNDAY",   "MONDAY", "TUESDAY",  "WEDNESDAY",
    "THURSDAY", "FRIDAY", "SATURDAY",
};

void play_audio(void* pData, unsigned char* pBuffer, int Length) {
  ((wall_clock*)pData)->play_chimes(pBuffer, Length);
}

wall_clock::wall_clock()
    : wnd_{nullptr},
      renderer_{nullptr},
      font_source_{nullptr},
      font_big_{nullptr},
      font_medium_{nullptr},
      font_small_{nullptr},
      display_{0},
      width_{0},
      height_{0},
      digit_width_{0},
      colon_width_{0},
      ampm_width_{0},
      time_width_{0},
      frame_time_{std::chrono::system_clock::duration(0)},
      now_{0},
      tense_{0},
      pitch_{0},
      volume_{0},
      text_color_{0, 0, 0, 0},
      background_{0, 0, 0, 0},
      dim_{true},
      whisper_{true},
      has_chime_{false},
      has_alarm_{false},
      date_{},
      time_24_{true},
      seconds_{true},
      next_alarm_{std::size_t(-1)},
      audio_device_{0},
      texture_second_{nullptr},
      size_second_{0, 0},
      texture_ampm_{nullptr},
      size_ampm_{0, 0},
      texture_time_{nullptr},
      size_time_{0, 0},
      texture_day_{nullptr},
      size_day_{0, 0},
      texture_date_{nullptr},
      size_date_{0, 0},
      texture_options_{nullptr},
      size_options_{0, 0},
      total_height_{0} {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) throw "SDL_INIT";
  SDL_ShowCursor(SDL_DISABLE);
  wnd_ = SDL_CreateWindow("wall_clock", SDL_WINDOWPOS_UNDEFINED,
                          SDL_WINDOWPOS_UNDEFINED, 0, 0,
                          SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
  if (!wnd_) {
    throw "SDL_create_window";
  }
  if (TTF_Init() < 0) {
    throw "TTF_Init";
  }
  font_source_ = SDL_RWFromConstMem(Font_ttf, Font_ttf_size);
  if (!font_source_) {
    throw "SDL_RWFromConstMem";
  }
  set_window();
  set_fonts();
  std::vector<int> sequence(13);
  std::iota(sequence.begin(), sequence.end(), 0);
  chimes_.insert(chimes_.end(), sequence.begin(), sequence.end());
  create_audio();
}

wall_clock::~wall_clock() {
  SDL_CloseAudioDevice(audio_device_);
  TTF_CloseFont(font_big_);
  TTF_CloseFont(font_medium_);
  TTF_CloseFont(font_small_);
  SDL_DestroyTexture(texture_second_);
  SDL_DestroyTexture(texture_time_);
  SDL_DestroyTexture(texture_day_);
  SDL_DestroyTexture(texture_date_);
  SDL_DestroyTexture(texture_options_);
  TTF_Quit();
  SDL_RWclose(font_source_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(wnd_);
  SDL_Quit();
}

void wall_clock::set_window() {
  SDL_Rect frame;
  if (SDL_GetDisplayBounds(display_, &frame) != 0) {
    throw "SDL_GetDisplayBounds";
  }
  SDL_DestroyRenderer(renderer_);
  if (SDL_SetWindowFullscreen(wnd_, 0) != 0) {
    throw "SDL_SetWindowFullscreen";
  }
  SDL_SetWindowPosition(wnd_, frame.x, frame.y);
  SDL_SetWindowSize(wnd_, frame.w, frame.h);
  if (SDL_SetWindowFullscreen(wnd_, SDL_WINDOW_FULLSCREEN) != 0) {
    throw "SDL_SetWindowFullscreen";
  }
  renderer_ = SDL_CreateRenderer(
      wnd_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer_) {
    throw "SDL_CreateRenderer";
  }
  if (SDL_GetRendererOutputSize(renderer_, &width_, &height_) != 0) {
    throw "SDL_GetRendererOutputSize";
  }
}

void wall_clock::set_fonts() {
  reset_big_font();
  auto text_width = std::max(digit_width_ * 6 + colon_width_ * 2, width_);
  SDL_RWseek(font_source_, 0, RW_SEEK_SET);
  TTF_CloseFont(font_medium_);
  font_medium_ = TTF_OpenFontRW(font_source_, false,
                                width_ * height_ * 2 / 9 / text_width);
  if (!font_medium_) {
    throw "TTF_OpenFont";
  }
  SDL_RWseek(font_source_, 0, RW_SEEK_SET);
  TTF_CloseFont(font_small_);
  font_small_ = TTF_OpenFontRW(font_source_, false,
                               width_ * height_ * 1 / 9 / text_width);
  if (!font_small_) {
    throw "TTF_OpenFont";
  }
  int space_width, a_width, p_width, m_width;
  if (TTF_SizeText(font_medium_, " ", &space_width, nullptr) < 0 ||
      TTF_SizeText(font_medium_, "A", &a_width, nullptr) < 0 ||
      TTF_SizeText(font_medium_, "P", &p_width, nullptr) < 0 ||
      TTF_SizeText(font_medium_, "M", &m_width, nullptr) < 0) {
    throw "TTF_SizeText";
  }
  ampm_width_ = space_width + std::max(a_width, p_width) + m_width;
  set_big_font();
}

void wall_clock::reset_big_font() {
  SDL_RWseek(font_source_, 0, RW_SEEK_SET);
  TTF_CloseFont(font_big_);
  font_big_ = TTF_OpenFontRW(font_source_, false, height_ * 4 / 9);
  if (!font_big_) {
    throw "TTF_OpenFont";
  }
  if (TTF_SizeText(font_big_, "0", &digit_width_, nullptr) < 0 ||
      TTF_SizeText(font_big_, ":", &colon_width_, nullptr) < 0) {
    throw "TTF_SizeText";
  }
  time_width_ = calculate_time_width();
}

void wall_clock::set_big_font() {
  SDL_RWseek(font_source_, 0, RW_SEEK_SET);
  TTF_CloseFont(font_big_);
  font_big_ = TTF_OpenFontRW(font_source_, false,
                             (width_ - ampm_width_) * height_ * 4 / 9 /
                                 (std::max(time_width_, width_) - ampm_width_));
  if (!font_big_) {
    throw "TTF_OpenFont";
  }
  if (TTF_SizeText(font_big_, "0", &digit_width_, nullptr) < 0 ||
      TTF_SizeText(font_big_, ":", &colon_width_, nullptr) < 0) {
    throw "TTF_SizeText";
  }
  time_width_ = calculate_time_width();
}

void wall_clock::create_audio() {
  SDL_AudioSpec Alarm;
  SDL_zero(Alarm);
  Alarm.freq = SEGMENT_COUNT * SAMPLE_COUNT;
  Alarm.format = AUDIO_F32SYS;
  Alarm.channels = 1;
  Alarm.samples = SAMPLE_COUNT;
  Alarm.callback = play_audio;
  Alarm.userdata = this;
  audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &Alarm, nullptr, 0);
  if (!audio_device_) {
    throw "SDL_create_audio";
  }
}

int wall_clock::calculate_time_width() {
  return digit_width_ * 4 + colon_width_ +
         (seconds_ ? digit_width_ * 2 + colon_width_ : 0) +
         (time_24_ ? 0 : ampm_width_);
}

void wall_clock::run() {
  for (;;) {
    auto now = std::chrono::system_clock::now();
    auto tmp =
        now.time_since_epoch().count() % std::chrono::system_clock::period::den;
    now = now - std::chrono::system_clock::duration(tmp);
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

int wall_clock::handle_event(SDL_Event* event) {
  int iResult = 0;
  switch (event->type) {
    case SDL_QUIT:
      iResult = -1;
      break;
    case SDL_KEYUP:
      switch (event->key.keysym.scancode) {
        case SDL_SCANCODE_ESCAPE:
          iResult = -1;
          break;
        case SDL_SCANCODE_SPACE:
          silent();
          break;
        case SDL_SCANCODE_R:
          test_bell();
          break;
      }
      break;
    case SDL_MOUSEBUTTONUP:
      iResult = -1;
      break;
    case SDL_FINGERUP:
      iResult = -1;
      break;
  }
  return iResult;
}

void wall_clock::read_config() {
  volume_ = 100;
  text_color_ = {255, 255, 255, 255};
  background_ = {0, 0, 0, 255};
  dim_ = true;
  whisper_ = true;
  has_chime_ = false;
  has_alarm_ = false;
  date_ = "%m/%d/%Y";
  time_24_ = true;
  seconds_ = true;
  next_alarm_ = std::size_t(-1);
  const char* home_directory = getenv(HOME);
  if (home_directory) {
    std::string conf_path{home_directory};
    conf_path += "/.clock.conf";
    std::ifstream config_file(conf_path.c_str());
    std::string config;
    std::set<std::size_t> alarms;
    while (std::getline(config_file, config)) {
      std::istringstream pair_stream{config};
      std::string key;
      pair_stream >> key;
      if (key == "alarm") {
        char colon;
        int hour, minute;
        if (pair_stream >> hour >> colon >> minute) {
          if (colon == ':' && hour < 24 && hour >= 0 && minute < 60 &&
              minute >= 0) {
            std::vector<std::size_t> alarm_weekdays;
            std::string weekday;
            while (pair_stream >> weekday) {
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
              } else {
                auto day =
                    std::find(weekdays_.begin(), weekdays_.end(), weekday);
                if (day != weekdays_.end()) {
                  alarm_weekdays.push_back(day - weekdays_.begin());
                }
              }
            }
            if (alarm_weekdays.empty()) {
              for (auto it = weekdays_.begin(); it != weekdays_.end(); ++it) {
                alarm_weekdays.push_back(it - weekdays_.begin());
              }
            }
            for (const auto& alarm_weekday : alarm_weekdays) {
              alarms.insert((alarm_weekday * 24 + hour) * 60 + minute);
            }
          }
        }
      } else if (key == "volume") {
        int volume;
        if (pair_stream >> volume && volume >= 0 && volume <= 100) {
          volume_ = volume;
        }
      } else if (key == "color") {
        int r, g, b;
        if (pair_stream >> r >> g >> b) {
          text_color_.r = r;
          text_color_.g = g;
          text_color_.b = b;
        }
      } else if (key == "background") {
        int r, g, b;
        if (pair_stream >> r >> g >> b) {
          background_.r = r;
          background_.g = g;
          background_.b = b;
        }
      } else if (key == "display") {
        int display;
        if (pair_stream >> display && display > 0 &&
            display <= SDL_GetNumVideoDisplays()) {
          display_ = display - 1;
        }
      } else if (key == "dim") {
        std::string dim;
        if (pair_stream >> dim) {
          if (dim == "true") {
            dim_ = true;
          } else if (dim == "false") {
            dim_ = false;
          }
        }
      } else if (key == "whisper") {
        std::string whisper;
        if (pair_stream >> whisper) {
          if (whisper == "true") {
            whisper_ = true;
          } else if (whisper == "false") {
            whisper_ = false;
          }
        }
      } else if (key == "chimes") {
        std::string chimes;
        if (pair_stream >> chimes) {
          if (chimes == "true") {
            has_chime_ = true;
          } else if (chimes == "false") {
            has_chime_ = false;
          }
        }
      } else if (key == "alarms") {
        std::string alarms;
        if (pair_stream >> alarms) {
          if (alarms == "true") {
            has_alarm_ = true;
          } else if (alarms == "false") {
            has_alarm_ = false;
          }
        }
      } else if (key == "date") {
        std::string date;
        if (pair_stream >> date) {
          if (date.size() <= 10) {
            date_ = date;
          }
        }
      } else if (key == "24-hour") {
        std::string time_24;
        if (pair_stream >> time_24) {
          if (time_24 == "true") {
            time_24_ = true;
          } else if (time_24 == "false") {
            time_24_ = false;
          }
        }
      } else if (key == "seconds") {
        std::string seconds;
        if (pair_stream >> seconds) {
          if (seconds == "true") {
            seconds_ = true;
          } else if (seconds == "false") {
            seconds_ = false;
          }
        }
      }
    }
    bool will_alarm = false;
    std::size_t alarm_time =
        (now_.tm_wday * 24 + now_.tm_hour) * 60 + now_.tm_min;
    auto alarm = alarms.lower_bound(alarm_time);
    if (alarm != alarms.end() && *alarm == alarm_time) {
      if (has_alarm_) {
        bell_alarm();
        will_alarm = true;
      }
      ++alarm;
    }
    if (has_chime_) {
      if (now_.tm_min == 0 && !will_alarm) {
        bell_chime();
      }
    }
    if (alarm == alarms.end()) {
      alarm = alarms.begin();
    }
    if (alarm != alarms.end()) {
      next_alarm_ = *alarm;
    }
  }
}

float wall_clock::get_volume() {
  return (volume_ / 100.0f) * (whisper_ ? tense_ : 1.0f) * 0.5f;
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
      if (SDL_GetWindowDisplayIndex(wnd_) != display_) {
        set_window();
        set_fonts();
      } else if (time_width_ != calculate_time_width()) {
        reset_big_font();
        set_big_font();
      }
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
    sSecond << ":" << std::setfill('0') << std::setw(2) << now_.tm_sec;
    draw_text(&texture_second_, &size_second_, sSecond.str(), font_big_,
              text_color_);
  }
  if (!second_only) {
    total_height_ = 0;

    if (!time_24_) {
      auto ampm = now_.tm_hour < 12 ? " AM" : " PM";
      draw_text(&texture_ampm_, &size_ampm_, ampm, font_medium_, text_color_);
    }

    std::stringstream sTime;
    sTime << std::setfill('0') << std::setw(2)
          << (time_24_ ? now_.tm_hour
                       : (now_.tm_hour % 12 != 0 ? now_.tm_hour % 12 : 12))
          << ":" << std::setfill('0') << std::setw(2) << now_.tm_min;
    draw_text(&texture_time_, &size_time_, sTime.str(), font_big_, text_color_);
    total_height_ += size_time_.y;

    std::stringstream sDay;
    sDay << wall_clock::weekdays_[now_.tm_wday];
    draw_text(&texture_day_, &size_day_, sDay.str(), font_medium_, text_color_);
    total_height_ += size_day_.y;

    std::stringstream sDate;
    sDate << std::put_time(&now_, date_.c_str());
    auto date = sDate.str();
    std::transform(date.begin(), date.end(), date.begin(), ::toupper);
    draw_text(&texture_date_, &size_date_, date, font_medium_, text_color_);
    total_height_ += size_date_.y;

    std::stringstream sInfo;
    sInfo << "\x5:" << (has_chime_ ? '\x7' : '\x8') << "  "
          << "\x6:" << (has_alarm_ ? '\x7' : '\x8') << " ";
    if (next_alarm_ != std::size_t(-1)) {
      sInfo << weekdays_[next_alarm_ / (60 * 24)][0]
            << weekdays_[next_alarm_ / (60 * 24)][1]
            << weekdays_[next_alarm_ / (60 * 24)][2] << " " << std::setfill('0')
            << std::setw(2) << next_alarm_ / 60 % 24 << ':' << std::setw(2)
            << next_alarm_ % 60;
    }
    draw_text(&texture_options_, &size_options_, sInfo.str(), font_small_,
              text_color_);
    total_height_ += size_options_.y;
  }
  if (SDL_SetRenderDrawColor(renderer_, background_.r, background_.g,
                             background_.b, background_.a) != 0 ||
      SDL_RenderClear(renderer_) != 0) {
    throw "Clear Background";
  }
  int iX;
  int iY = (height_ - total_height_) / 5;

  iX = (width_ - size_time_.x - (seconds_ ? size_second_.x : 0) -
        (time_24_ ? 0 : size_ampm_.x)) /
       2;
  render_texture(texture_time_, &size_time_, iX, iY);
  iX += size_time_.x;
  if (seconds_) {
    render_texture(texture_second_, &size_second_, iX, iY);
    iX += size_second_.x;
  }
  if (!time_24_) {
    render_texture(texture_ampm_, &size_ampm_, iX,
                   iY + (size_time_.y - size_ampm_.y) / 2);
    iX += size_ampm_.x;
  }
  iY += size_time_.y + (height_ - total_height_) / 5;
  iX = (width_ - size_day_.x) / 2;
  render_texture(texture_day_, &size_day_, iX, iY);
  iY += size_day_.y + (height_ - total_height_) / 5;
  iX = (width_ - size_date_.x) / 2;
  render_texture(texture_date_, &size_date_, iX, iY);
  iY += size_date_.y + (height_ - total_height_) / 5;
  iX = (width_ - size_options_.x) / 2;
  render_texture(texture_options_, &size_options_, iX, iY);
  iY += size_date_.y + (height_ - total_height_) / 5;
  SDL_RenderPresent(renderer_);
}

void wall_clock::draw_text(SDL_Texture** texture, SDL_Point* size,
                           const std::string& text, TTF_Font* font,
                           const SDL_Color& color) {
  SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
  if (!surface) {
    throw "TTF_RenderText_Solid";
  }
  SDL_DestroyTexture(*texture);
  *texture = SDL_CreateTextureFromSurface(renderer_, surface);
  if (!*texture) {
    throw "SDL_CreateTextureFromSurface";
  }
  if (SDL_QueryTexture(*texture, nullptr, nullptr, &size->x, &size->y) != 0) {
    throw "SDL_QueryTexture";
  }
  SDL_FreeSurface(surface);
}

void wall_clock::render_texture(SDL_Texture* texture, const SDL_Point* size,
                                const int x, const int y) {
  SDL_Rect dest{x, y, size->x, size->y};
  if (SDL_RenderCopy(renderer_, texture, nullptr, &dest) != 0) {
    throw "SDL_RenderCopy";
  }
}

void wall_clock::bell_alarm() {
  SDL_LockAudioDevice(audio_device_);
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
  SDL_UnlockAudioDevice(audio_device_);
  SDL_PauseAudioDevice(audio_device_, 0);
}

void wall_clock::bell_chime() {
  int count = now_.tm_hour % 12;
  if (count == 0) {
    count = 12;
  }
  SDL_LockAudioDevice(audio_device_);
  if (strikes_.empty()) {
    for (int i = 0; i < count; ++i) {
      strikes_.push_back(
          {-int((i * 1.5f + 0.0f) * SEGMENT_COUNT) * SAMPLE_COUNT, get_volume(),
           pitch_});
    }
  }
  SDL_UnlockAudioDevice(audio_device_);
  SDL_PauseAudioDevice(audio_device_, 0);
}

void wall_clock::bell_test() {
  SDL_LockAudioDevice(audio_device_);
  if (strikes_.empty()) {
    for (int i = 0; i < 2; ++i) {
      strikes_.push_back(
          {-int((i * 2.0f + 0.0f) * SEGMENT_COUNT) * SAMPLE_COUNT, get_volume(),
           pitch_});
    }
  }
  SDL_UnlockAudioDevice(audio_device_);
  SDL_PauseAudioDevice(audio_device_, 0);
}

void wall_clock::silent() {
  SDL_LockAudioDevice(audio_device_);
  strikes_.remove_if([](auto& strike) { return strike.pos <= 0; });
  SDL_UnlockAudioDevice(audio_device_);
}

void wall_clock::test_bell() { bell_test(); }

void wall_clock::play_chimes(unsigned char* buffer, int length) {
  SDL_memset(buffer, 0, length);
  SDL_LockAudioDevice(audio_device_);
  if (strikes_.empty())
    SDL_PauseAudioDevice(audio_device_, 1);
  else {
    for (auto it = strikes_.begin(); it != strikes_.end();) {
      if (chimes_[it->pitch].play(it->volume / 1.7f, it->pos,
                                  reinterpret_cast<float*>(buffer),
                                  length / sizeof(float))) {
        ++it;
      } else {
        it = strikes_.erase(it);
      }
    }
  }
  SDL_UnlockAudioDevice(audio_device_);
}
