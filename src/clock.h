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

	struct CHIME_INFO
	{
		float delay;
		float volume;
		int pitch;
	};

private:
	SDL_Window* m_pWnd;
	SDL_Renderer *m_pRen;
	unsigned char m_Color;
	SDL_RWops* m_FontSource;
	TTF_Font* m_FontBig;
	TTF_Font* m_FontMedium;
	TTF_Font* m_FontSmall;
	int m_Width;
	std::chrono::system_clock::time_point m_frameTime;
	std::tm* m_pNow;
	float m_Tence;
	bool m_Chime;
	bool m_Alarm;
	SDL_AudioDeviceID m_Audio;
	std::list<chime> m_Dings;

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
	void CheckBell();
	void DrawText(const std::string & sText, TTF_Font* const pFont, const SDL_Color & color, int * iY);
	int get_pitch();
	void Bell(std::list<CHIME_INFO> chimes);
	void ColorUp();
	void ColorDown();
	void Silent();
	void ToggleAlarm();
	void ToggleChime();
	void RingBell();
};
