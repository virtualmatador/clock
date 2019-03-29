#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <chrono>
#include <string>
#include <list>

extern const unsigned char _binary_res_Font_ttf_end[];
extern const unsigned char _binary_res_Font_ttf_start[];


class Clock
{
private:
	SDL_Window* m_pWnd;
	SDL_Renderer *m_pRen;
	unsigned char m_Color;
	SDL_RWops* m_FontSource;
	TTF_Font* m_FontTime;
	TTF_Font* m_FontDate;
	int m_iWidth;
	std::chrono::system_clock::time_point m_frameTime;
	std::tm* m_pNow;
	bool m_Chime;
	bool m_Alarm;
	SDL_AudioDeviceID m_Audio;
	std::list<int> m_Dings;

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
	void Bell(int Count);
	void ColorUp();
	void ColorDown();
	void Silent();
	void ToggleAlarm();
	void ToggleChime();
	void RingBell();
};
