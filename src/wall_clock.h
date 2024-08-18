#ifndef SRC_WALL_CLOCK_H
#define SRC_WALL_CLOCK_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "chime.h"

class wall_clock {
  struct STRIKE {
    int pos;
    float volume;
    int pitch;
  };

 private:
  std::chrono::system_clock::time_point frame_time_;
  std::tm now_;

  SDL_Window* wnd_;
  SDL_Renderer* renderer_;
  SDL_RWops* font_source_;
  TTF_Font* font_big_;
  TTF_Font* font_medium_;
  TTF_Font* font_small_;
  SDL_Texture* texture_second_;
  SDL_Point size_second_;
  SDL_Texture* texture_time_;
  SDL_Point size_time_;
  SDL_Texture* texture_ampm_;
  SDL_Point size_ampm_;
  SDL_Texture* texture_day_;
  SDL_Point size_day_;
  SDL_Texture* texture_date_;
  SDL_Point size_date_;
  SDL_Texture* texture_options_;
  SDL_Point size_options_;
  int total_height_;
  int width_;
  int height_;
  int digit_width_;
  int colon_width_;
  int ampm_width_;
  int time_width_;

  SDL_AudioDeviceID audio_device_;
  std::vector<chime> chimes_;
  float tense_;
  int pitch_;
  int volume_;
  int display_;
  SDL_Color text_color_;
  SDL_Color background_;
  bool dim_;
  bool whisper_;
  bool has_chime_;
  bool has_alarm_;
  std::string date_;
  bool time_24_;
  bool seconds_;
  bool pad_hour_;
  bool pad_minute_;
  bool pad_second_;
  bool pad_year_;
  bool pad_month_;
  bool pad_day_;
  std::set<std::size_t> alarms_;
  std::size_t next_alarm_;
  std::list<STRIKE> strikes_;
  std::map<std::string, std::function<void(std::istream&)>> config_handlers_;

 private:
  inline static std::array weekdays_ = {
      "SUNDAY",   "MONDAY", "TUESDAY",  "WEDNESDAY",
      "THURSDAY", "FRIDAY", "SATURDAY",
  };
  inline static std::array months_ = {
      "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
  };

 public:
  wall_clock();
  ~wall_clock();
  void run();
  void play_chimes(unsigned char* buffer, int length);

 private:
  void set_window();
  void set_fonts();
  void reset_big_font();
  void set_big_font();
  void create_audio();
  void set_config_handlers();
  int calculate_time_width();
  int handle_event(SDL_Event* event);
  void tick();
  void read_config();
  float get_volume();
  void redraw(const bool second_only);
  void draw_text(SDL_Texture** texture, SDL_Point* size,
                 const std::string& text, TTF_Font* font,
                 const SDL_Color& color);
  void render_texture(SDL_Texture* texture, const SDL_Point* size, const int x,
                      const int y);
  void bell_alarm();
  void bell_chime();
  void bell_test();
  void silent();
  void test_bell();
};

#endif  // !SRC_WALL_CLOCK_H
