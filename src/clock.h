#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <chrono>
#include <string>

class Clock
{
private:
	SDL_Window* m_pWnd;
	SDL_Renderer *m_pRen;
	unsigned char m_Color;
	TTF_Font* m_FontTime;
	TTF_Font* m_FontDate;
	int m_iWidth;
	std::chrono::system_clock::time_point m_frameTime;
	static const char* m_WeekDays[];

public:
	Clock();
	~Clock();
	void Run();

private:
	void CreateWindow();
	int HandleEvent(SDL_Event* pEvent);
	void Tick();
	void DrawText(const std::string & sText, TTF_Font* const pFont, const SDL_Color & color, int * iY);
	void ColorUp();
	void ColorDown();
};
