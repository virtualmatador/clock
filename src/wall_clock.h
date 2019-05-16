#include "chime.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <chrono>
#include <string>
#include <list>

#define TENCE_MIN 0.25f

extern const unsigned char _binary_res_Font_ttf_end[];
extern const unsigned char _binary_res_Font_ttf_start[];

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
	std::tm* now_;

	SDL_Window* wnd_;
	SDL_Renderer *renderer_;
	SDL_RWops* font_source_;
	TTF_Font* font_big_;
	TTF_Font* font_medium_;
	TTF_Font* font_small_;
	unsigned char shade_;
	int width_;

	SDL_AudioDeviceID audio_device_;
	std::vector<chime> chimes_;
	float tence_;
	int pitch_;
	bool has_chime_;
	bool has_alarm_;
	std::list<STRIKE> strikes_;

private:
	static const char* weekdays_[];

public:
	wall_clock();
	~wall_clock();
	void run();
	void play_chimes(unsigned char* pBuffer, int Length);

private:
	void create_window();
	void create_audio();
	int handle_event(SDL_Event* pEvent);
	void tick();
	void redraw();
	void bell_alarm();
	void bell_hour();
	void bell_test();
	void check_bell();
	void draw_text(const std::string & sText, TTF_Font* const pFont, const SDL_Color & color, int * iY);
	void shade_up();
	void shade_down();
	void silent();
	void toggle_alarm();
	void toggle_chime();
	void test_bell();
};
