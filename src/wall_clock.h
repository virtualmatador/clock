#ifndef SRC_WALL_CLOCK_H
#define SRC_WALL_CLOCK_H

#include <chrono>
#include <list>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "chime.h"

class wall_clock
{

	struct STRIKE
	{
		int pos;
		float volume;
		int pitch;
	};

private:
	std::chrono::system_clock::time_point frame_time_;
	std::tm now_;

	SDL_Window* wnd_;
	SDL_Renderer *renderer_;
	SDL_RWops* font_source_;
	TTF_Font* font_big_;
	TTF_Font* font_medium_;
	TTF_Font* font_small_;
	SDL_Texture* texture_second_;
	SDL_Texture* texture_time_;
	SDL_Texture* texture_day_;
	SDL_Texture* texture_date_;
	SDL_Texture* texture_options_;
	int volume_;
	SDL_Color text_color_;
	SDL_Color background_;
	int display_;
	int width_;
	int height_;
	int digit_width_;
	int colon_width_;

	SDL_AudioDeviceID audio_device_;
	std::vector<chime> chimes_;
	float tence_;
	int pitch_;
	bool dim_;
	bool has_chime_;
	bool has_alarm_;
	std::string date_;
	std::size_t next_alarm_;
	std::list<STRIKE> strikes_;

private:
	static std::vector<const char*> weekdays_;

public:
	wall_clock();
	~wall_clock();
	void run();
	void play_chimes(unsigned char* buffer, int length);

private:
	void set_window();
	void create_audio();
	int handle_event(SDL_Event* event);
	void tick();
	void read_config();
	float get_volume();
	void redraw(const bool second_only);
	void draw_text(SDL_Texture** texture, const std::string & text, TTF_Font* font, const SDL_Color & color);
	int render_texture(SDL_Texture* texture, const int x, const int y);
	void bell_alarm();
	void bell_chime();
	void bell_test();
	void silent();
	void test_bell();
};

#endif // !SRC_WALL_CLOCK_H
