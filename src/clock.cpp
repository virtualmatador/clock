#include "clock.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

Clock::Clock()
	: m_pWnd{nullptr}
	, m_pRen{nullptr}
	, m_Color{255, 255, 255}
	, m_iWidth{0}
	, m_frameTime{std::chrono::nanoseconds(0)}
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw "SDL_INIT";
	CreateWindow();
	if (TTF_Init() < 0)
		throw "TTF_Init";
	m_Font = TTF_OpenFont("/usr/share/fonts/truetype/crosextra/Caladea-Bold.ttf", 420);
	if (!m_Font)
		throw "TTF_OpenFont";
}

Clock::~Clock()
{
	TTF_CloseFont(m_Font);
	TTF_Quit();
	SDL_DestroyRenderer(m_pRen);
	SDL_DestroyWindow(m_pWnd);
	SDL_Quit();
}

void Clock::CreateWindow()
{
	SDL_ShowCursor(SDL_DISABLE);
	m_pWnd = SDL_CreateWindow("Clock", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (!m_pWnd)
		throw "SDL_CreateWindow";
	SDL_GetWindowSize(m_pWnd, &m_iWidth, nullptr);
	m_pRen = SDL_CreateRenderer(m_pWnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!m_pRen)
		throw "SDL_CreateRenderer";
}

void Clock::Run()
{
	for(;;)
	{
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		long tmp = now.time_since_epoch().count() % 1000000000;
		now -= std::chrono::nanoseconds(tmp);
		if (now != m_frameTime)
		{
			m_frameTime = now;
			Tick();
		}
		else
		{
			SDL_Event event;
			if (SDL_WaitEventTimeout(&event, (1000000000 - tmp) / 1000000) == 1)
			{
				if (HandleEvent(&event) < 0)
					break;
			}
		}
	}
}

int Clock::HandleEvent(SDL_Event* pEvent)
{
	switch (pEvent->type)
	{
	case SDL_QUIT:
		return -1;
		break;
	}
	return 0;
}

void Clock::Tick()
{
	std::time_t t = std::chrono::system_clock::to_time_t(m_frameTime);
	std::tm* now = std::localtime(&t);

	std::stringstream sTime;
	sTime <<
		std::setfill('0') << std::setw(2) << now->tm_hour << ":" <<
		std::setfill('0') << std::setw(2) << now->tm_min << ":" <<
		std::setfill('0') << std::setw(2) << now->tm_sec;

	//now->tm_year + 1900;
	//now->tm_mon + 1;
	//now->tm_mday;

	if (SDL_SetRenderDrawColor(m_pRen, 0, 0, 0, 0) == 0)
	{
		if (SDL_RenderClear(m_pRen) == 0)
		{
			SDL_Surface* surfaceMessage = TTF_RenderText_Solid(m_Font, sTime.str().c_str(), m_Color);
			if (surfaceMessage)
			{
				SDL_Texture* curTime = SDL_CreateTextureFromSurface(m_pRen, surfaceMessage);
				if (curTime)
				{
					int w, h;
					if (SDL_QueryTexture(curTime, nullptr, nullptr, &w, &h) == 0)
					{
						SDL_Rect dest{(m_iWidth - w) / 2, 0, w, h};
						SDL_RenderCopy(m_pRen, curTime, nullptr, &dest);
						SDL_DestroyTexture(curTime);
						SDL_FreeSurface(surfaceMessage);
						SDL_RenderPresent(m_pRen);
					}
				}
			}
		}
	}
}

int main(int argc, char* argv[])
{
	try
	{
		Clock clock;
		clock.Run();
	}
	catch(const char* szE)
	{
		std::cout << "Exception: " << szE << std::endl;
		return -1;
	}
	return 0;
}
