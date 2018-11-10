#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <chrono>

class Clock
{
private:
	SDL_Window* m_pWnd;
	SDL_Renderer *m_pRen;
	SDL_Color m_Color;
	int m_iWidth;
	TTF_Font* m_Font;
	std::chrono::system_clock::time_point m_frameTime;

public:
	Clock();
	~Clock();
	void Run();

private:
	void CreateWindow();
	int HandleEvent(SDL_Event* pEvent);
	void Tick();
};
