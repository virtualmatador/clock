#include "clock.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

const char* Clock::m_WeekDays[] =
{
	"SUN",
	"MON",
	"TUE",
	"WED",
	"THU",
	"FRI",
	"SAT",
};

Clock::Clock()
	: m_pWnd{nullptr}
	, m_pRen{nullptr}
	, m_Color{255}
	, m_iWidth{0}
	, m_frameTime{std::chrono::nanoseconds(0)}
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw "SDL_INIT";
	CreateWindow();
	if (TTF_Init() < 0)
		throw "TTF_Init";
	m_FontTime = TTF_OpenFont("/usr/share/fonts/truetype/crosextra/Carlito-Bold.ttf", 500);
	if (!m_FontTime)
		throw "TTF_OpenFont";
	m_FontDate = TTF_OpenFont("/usr/share/fonts/truetype/crosextra/Carlito-Bold.ttf", 240);
	if (!m_FontDate)
		throw "TTF_OpenFont";
}

Clock::~Clock()
{
	TTF_CloseFont(m_FontTime);
	TTF_CloseFont(m_FontDate);
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
	case SDL_KEYDOWN:
		switch(pEvent->key.keysym.scancode)
		{
		case SDL_SCANCODE_ESCAPE:
			if (pEvent->key.repeat == 0)
				return -1;
			break;
		case SDL_SCANCODE_UP:
			ColorUp();
			break;
		case SDL_SCANCODE_DOWN:
			ColorDown();
			break;
		}
	}
	return 0;
}

void Clock::Tick()
{
	std::time_t t = std::chrono::system_clock::to_time_t(m_frameTime);
	std::tm* now = std::localtime(&t);

	int iY = 0;
	if (SDL_RenderClear(m_pRen) == 0)
	{
		unsigned char iColor = m_Color * (std::sin(2.0 * std::atan(1) * 4.0 *
				((now->tm_hour - 6) * 60 + now->tm_min) / 24.0 / 60.0) * 0.8 / 2.0 + 0.8 / 2.0 + 0.2);
		SDL_Color color{iColor, iColor, iColor};
		std::stringstream sTime;
		sTime <<
			std::setfill('0') << std::setw(2) << now->tm_hour << ":" <<
			std::setfill('0') << std::setw(2) << now->tm_min << ":" <<
			std::setfill('0') << std::setw(2) << now->tm_sec;
		DrawText(sTime.str(), m_FontTime, color, &iY);
		std::stringstream sDay;
		sDay << Clock::m_WeekDays[now->tm_wday];
		DrawText(sDay.str(), m_FontDate, color, &iY);
		std::stringstream sDate;
		sDate <<
			now->tm_year + 1900 << "/" <<
			std::setfill('0') << std::setw(2) << now->tm_mon + 1 << "/" <<
			std::setfill('0') << std::setw(2) << now->tm_mday;
		DrawText(sDate.str(), m_FontDate, color, &iY);
	}
	SDL_RenderPresent(m_pRen);
}

void Clock::DrawText(const std::string & sText, TTF_Font* const pFont, const SDL_Color & color, int * piY)
{
	SDL_Surface* surface = TTF_RenderText_Solid(pFont, sText.c_str(), color);
	if (surface)
	{
		SDL_Texture* texture = SDL_CreateTextureFromSurface(m_pRen, surface);
		if (texture)
		{
			int w, h;
			if (SDL_QueryTexture(texture, nullptr, nullptr, &w, &h) == 0)
			{
				SDL_Rect dest{(m_iWidth - w) / 2, *piY, w, h};
				SDL_RenderCopy(m_pRen, texture, nullptr, &dest);
				SDL_DestroyTexture(texture);
				SDL_FreeSurface(surface);
				*piY += h;
			}
		}
	}
}

void Clock::ColorUp()
{
	if (m_Color < 255)
		++m_Color;
	Tick();
}

void Clock::ColorDown()
{
	if (m_Color > 0)
		--m_Color;
	Tick();
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
