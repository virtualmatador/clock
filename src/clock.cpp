#include "clock.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

#define AUDIO_FREQUENCY 48000
#define SEGMENT_COUNT 32

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

void PlayDing(void* pData, unsigned char* pBuffer, int Length)
{
	((Clock*)pData)->PlayDing(pBuffer, Length);
}

Clock::Clock()
	: m_pWnd{nullptr}
	, m_pRen{nullptr}
	, m_Color{255}
	, m_iWidth{0}
	, m_frameTime{std::chrono::nanoseconds(0)}
	, m_Audio{0}
	, m_Dings{0}
	, m_Alarm{false}
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		throw "SDL_INIT";
	CreateWindow();
	if (TTF_Init() < 0)
		throw "TTF_Init";
	m_FontSource = SDL_RWFromConstMem(_binary_res_Carlito_Bold_ttf_start,
		_binary_res_Carlito_Bold_ttf_end - _binary_res_Carlito_Bold_ttf_start);
	m_FontTime = TTF_OpenFontRW(m_FontSource, false, m_iWidth / 4);
	if (!m_FontTime)
		throw "TTF_OpenFont";
	m_FontDate = TTF_OpenFontRW(m_FontSource, false, m_iWidth / 8);
	if (!m_FontDate)
		throw "TTF_OpenFont";
	CreateAudio();
}

Clock::~Clock()
{
	SDL_CloseAudioDevice(m_Audio);
	TTF_CloseFont(m_FontTime);
	TTF_CloseFont(m_FontDate);
	TTF_Quit();
	SDL_RWclose(m_FontSource);
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

void Clock::CreateAudio()
{
	SDL_AudioSpec Alarm;
	SDL_zero(Alarm);
    Alarm.freq = AUDIO_FREQUENCY;
    Alarm.format = AUDIO_S16SYS;
    Alarm.channels = 1;
    Alarm.samples = AUDIO_FREQUENCY / SEGMENT_COUNT;
    Alarm.callback = ::PlayDing;
    Alarm.userdata = this;
	m_Audio = SDL_OpenAudioDevice(nullptr, 0, &Alarm, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE);
 	if (!m_Audio)
		throw "SDL_CreateAudio";
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
			Tick(false);
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
	int iResult = 0;
	switch (pEvent->type)
	{
	case SDL_QUIT:
		iResult = -1;
		break;
	case SDL_KEYDOWN:
		switch(pEvent->key.keysym.scancode)
		{
		case SDL_SCANCODE_ESCAPE:
			if (pEvent->key.repeat == 0)
				iResult = -1;
			break;
		case SDL_SCANCODE_UP:
			ColorUp();
			break;
		case SDL_SCANCODE_DOWN:
			ColorDown();
			break;
		case SDL_SCANCODE_SPACE:
			Silent();
			break;
		}
		break;
	case SDL_MOUSEBUTTONUP:
		iResult = -1;
		break;
	case SDL_FINGERUP:
		iResult = -1;
		break;
	}
	return iResult;
}

void Clock::Tick(bool ForceUpdate)
{
	static std::time_t tPre{0};
	std::time_t t = std::chrono::system_clock::to_time_t(m_frameTime);
	if (ForceUpdate || tPre != t)
	{
		std::tm* ptmNow = std::localtime(&t);
		int iY = 0;
		if (SDL_RenderClear(m_pRen) == 0)
		{
			unsigned char iColor;
			if (ptmNow->tm_hour < 6 || ptmNow->tm_hour >= 18)
				iColor = m_Color * 0.2;
			else
				iColor = m_Color * (std::sin(2.0 * std::atan(1) * 4.0 *
					((ptmNow->tm_hour - 9) * 60 + ptmNow->tm_min) / 12.0 / 60.0) * 0.8 / 2.0 + 0.8 / 2.0 + 0.2);
			SDL_Color color{iColor, iColor, iColor};
			std::stringstream sTime;
			sTime <<
				std::setfill('0') << std::setw(2) << ptmNow->tm_hour << ":" <<
				std::setfill('0') << std::setw(2) << ptmNow->tm_min << ":" <<
				std::setfill('0') << std::setw(2) << ptmNow->tm_sec;
			DrawText(sTime.str(), m_FontTime, color, &iY);
			std::stringstream sDay;
			sDay << Clock::m_WeekDays[ptmNow->tm_wday];
			DrawText(sDay.str(), m_FontDate, color, &iY);
			std::stringstream sDate;
			sDate <<
				ptmNow->tm_year + 1900 << "/" <<
				std::setfill('0') << std::setw(2) << ptmNow->tm_mon + 1 << "/" <<
				std::setfill('0') << std::setw(2) << ptmNow->tm_mday;
			DrawText(sDate.str(), m_FontDate, color, &iY);
		}
		SDL_RenderPresent(m_pRen);
		static int MinPre = -1;
		if (tPre != t)
		{
			if (MinPre != ptmNow->tm_min)
			{
				m_Alarm = false;
				std::ifstream Settings(".clock");
				std::string Time;
				while (!m_Alarm && std::getline(Settings, Time))
				{
					std::istringstream iss(Time);
					char colon;
					int Hour, Minute;
					if (iss >> Hour >> colon >> Minute)
					{
						if (colon == ':' && Hour == ptmNow->tm_hour && Minute == ptmNow->tm_min)
							m_Alarm = true;
					}
				}
				MinPre = ptmNow->tm_min;
			}
			if (m_Alarm)
			{
				m_Dings = 4;
				SDL_PauseAudioDevice(m_Audio, 0);
			}
		}
		tPre = t;
	}
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
	Tick(true);
}

void Clock::ColorDown()
{
	if (m_Color > 0)
		--m_Color;
	Tick(true);
}

void Clock::Silent()
{
	m_Alarm = false;
}

void Clock::PlayDing(unsigned char* pBuffer, int Length)
{
	static int m_AudioPos = 0;
	if (m_Dings > 0)
	{
		float fVolumeBegin, fVolumeChange;
		if (m_AudioPos == 0)
		{
			fVolumeBegin = 0;
			fVolumeChange = 1;
			++m_AudioPos;
		}
		else if (m_AudioPos < 3)
		{
			fVolumeBegin = 1;
			fVolumeChange = 0;
			++m_AudioPos;
		}
		else
		{
			fVolumeBegin = 1;
			fVolumeChange = -1;
			m_AudioPos = 0;
			--m_Dings;
		}
		for (int i = 0; i < Length / sizeof(short); ++i)
			((short*)pBuffer)[i] = 24000.0f * (fVolumeBegin + fVolumeChange * i / (Length / sizeof(short))) *
			(
				sin(float(1046 / SEGMENT_COUNT * i) / (Length / sizeof(short)) * 8.0f * atan(1.0f)) * 0.5f+
				sin(float(2093 / SEGMENT_COUNT * i) / (Length / sizeof(short)) * 8.0f * atan(1.0f)) * 1.0f+
				0.0f
			) / 1.5f;
	}
	else
	{
		SDL_memset(pBuffer, 0, Length);
		SDL_PauseAudioDevice(m_Audio, 1);
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
