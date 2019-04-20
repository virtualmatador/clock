#include "chime.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <chrono>
#include <string>
#include <list>

#define TENCE_MIN 0.25f

extern const unsigned char _binary_res_Font_ttf_end[];
extern const unsigned char _binary_res_Font_ttf_start[];

class Clock
{

	struct STRIKE
	{
		int pos;
		float volume;
		int pitch;
	};

private:
	std::chrono::system_clock::time_point m_frameTime;
	std::tm* m_pNow;

	SDL_Window* m_pWnd;
	SDL_Renderer *m_pRen;
	SDL_RWops* m_FontSource;
	TTF_Font* m_FontBig;
	TTF_Font* m_FontMedium;
	TTF_Font* m_FontSmall;
	unsigned char m_Color;
	int m_Width;

	SDL_AudioDeviceID m_Audio;
	chime chimes_[13];
	float m_Tence;
	int m_Pitch;
	bool m_Chime;
	bool m_Alarm;
	std::list<STRIKE> strikes_;

private:
	static const char* m_WeekDays[];

public:
	Clock();
	~Clock();
	void Run();
	void PlayDing(unsigned char* pBuffer, int Length);

private:
	void CreateWindow();
	void CreateAudio();
	int HandleEvent(SDL_Event* pEvent);
	void Tick();
	void Redraw();
	void bell_alarm();
	void bell_hour();
	void bell_test();
	void CheckBell();
	void DrawText(const std::string & sText, TTF_Font* const pFont, const SDL_Color & color, int * iY);
	void ColorUp();
	void ColorDown();
	void Silent();
	void ToggleAlarm();
	void ToggleChime();
	void RingBell();
};
