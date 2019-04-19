#include "clock.h"

#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

const char* Clock::m_WeekDays[] =
{
	"SUNDAY",
	"MONDAY",
	"TUESDAY",
	"WEDNESDAY",
	"THURSDAY",
	"FRIDAY",
	"SATURDAY",
};

void PlayDing(void* pData, unsigned char* pBuffer, int Length)
{
	((Clock*)pData)->PlayDing(pBuffer, Length);
}

Clock::Clock()
	: m_pWnd{nullptr}
	, m_pRen{nullptr}
	, m_Color{255}
	, m_Width{0}
	, m_frameTime{std::chrono::nanoseconds(0)}
	, m_pNow{nullptr}
	, m_Tence{TENCE_MIN}
	, m_Pitch{0}
	, m_Chime{false}
	, m_Alarm{false}
	, m_Audio{0}
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		throw "SDL_INIT";
	CreateWindow();
	if (TTF_Init() < 0)
		throw "TTF_Init";
	m_FontSource = SDL_RWFromConstMem(_binary_res_Font_ttf_start,
		_binary_res_Font_ttf_end - _binary_res_Font_ttf_start);	
	SDL_RWseek(m_FontSource, 0, RW_SEEK_SET);
	m_FontBig = TTF_OpenFontRW(m_FontSource, false, m_Width / 4);
	if (!m_FontBig)
		throw "TTF_OpenFont";
	SDL_RWseek(m_FontSource, 0, RW_SEEK_SET);
	m_FontMedium = TTF_OpenFontRW(m_FontSource, false, m_Width / 8);
	if (!m_FontMedium)
		throw "TTF_OpenFont";
	m_FontSmall = TTF_OpenFontRW(m_FontSource, false, m_Width / 16);
	if (!m_FontSmall)
		throw "TTF_OpenFont";
	CreateAudio();
}

Clock::~Clock()
{
	SDL_CloseAudioDevice(m_Audio);
	TTF_CloseFont(m_FontBig);
	TTF_CloseFont(m_FontMedium);
	TTF_CloseFont(m_FontSmall);
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
	SDL_GetWindowSize(m_pWnd, &m_Width, nullptr);
	m_pRen = SDL_CreateRenderer(m_pWnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!m_pRen)
		throw "SDL_CreateRenderer";
}

void Clock::CreateAudio()
{
	SDL_AudioSpec Alarm;
	SDL_zero(Alarm);
    Alarm.freq = SEGMENT_COUNT * SAMPLE_COUNT;
    Alarm.format = AUDIO_F32SYS;
    Alarm.channels = 1;
    Alarm.samples = SAMPLE_COUNT;
    Alarm.callback = ::PlayDing;
    Alarm.userdata = this;
	m_Audio = SDL_OpenAudioDevice(nullptr, 0, &Alarm, nullptr, 0);
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
		case SDL_SCANCODE_A:
			ToggleAlarm();
			break;
		case SDL_SCANCODE_C:
			ToggleChime();
			break;
		case SDL_SCANCODE_R:
			RingBell();
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

void Clock::Tick()
{
	static std::time_t tPre{0};
	std::time_t t = std::chrono::system_clock::to_time_t(m_frameTime);
	if (tPre != t)
	{
		m_pNow = std::localtime(&t);
		static int MinPre = -1;
		if (MinPre != m_pNow->tm_min)
		{
			m_Tence = std::max(0, 6 * 60 - std::abs(m_pNow->tm_hour * 60 + m_pNow->tm_min - 12 * 60))
				/ 360.0f * (1.0f - TENCE_MIN) + TENCE_MIN;
			m_Pitch = 12 - std::abs(m_pNow->tm_hour - 12);
			CheckBell();
			MinPre = m_pNow->tm_min;
		}
		Redraw();
		tPre = t;
	}
}

void Clock::Redraw()
{
	int iY = 0;
	if (SDL_RenderClear(m_pRen) == 0)
	{
		SDL_Color color
		{
			(unsigned char)(m_Color * m_Tence),
			(unsigned char)(m_Color * m_Tence),
			(unsigned char)(m_Color * m_Tence),
		};

		std::stringstream sTime;
		sTime <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_hour << ":" <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_min << ":" <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_sec;
		DrawText(sTime.str(), m_FontBig, color, &iY);

		std::stringstream sDay;
		sDay << Clock::m_WeekDays[m_pNow->tm_wday];
		DrawText(sDay.str(), m_FontMedium, color, &iY);

		std::stringstream sDate;
		sDate <<
			m_pNow->tm_year + 1900 << "/" <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_mon + 1 << "/" <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_mday;
		DrawText(sDate.str(), m_FontMedium, color, &iY);

		std::stringstream sInfo;
		sInfo << "\x5:" << (m_Chime ? '\x7' : '\x8') << "  " << "\x6:" << (m_Alarm ? '\x7' : '\x8');
		DrawText(sInfo.str(), m_FontSmall, color, &iY);
	}
	SDL_RenderPresent(m_pRen);
}

void Clock::bell_alarm()
{
	std::list<CHIME_INFO> chimes;
	for (int i = 0; i < 12; ++i)
	{
		chimes.push_back({i * 3.0f + 0.0f, m_Tence * (i + 3) / 15.0f, i + 2});
		chimes.push_back({i * 3.0f + 0.5f, m_Tence * (i + 2) / 15.0f, i + 1});
		chimes.push_back({i * 3.0f + 1.0f, m_Tence * (i + 1) / 15.0f, i + 0});
	}
	Bell(chimes);
}

void Clock::bell_hour()
{
	int count = m_pNow->tm_hour % 12;
	if (count == 0)
		count = 12;
	std::list<CHIME_INFO> chimes;
	for (int i = 0; i < count; ++i)
		chimes.push_back({i * 1.5f, m_Tence / 1.5f, m_Pitch});
	Bell(chimes);
}

void Clock::bell_test()
{
	std::list<CHIME_INFO> chimes;
	for (int i = 0; i < 2; ++i)
		chimes.push_back({i * 2.0f, m_Tence / 1.5f, m_Pitch});
	Bell(chimes);
}

void Clock::CheckBell()
{
	bool Alarm = false;
	if (m_Alarm)
	{
		std::ifstream Settings(".clock");
		std::string Time;
		while (std::getline(Settings, Time))
		{
			std::istringstream iss(Time);
			char colon;
			int Hour, Minute;
			if (iss >> Hour >> colon >> Minute)
			{
				if (colon == ':' && Hour == m_pNow->tm_hour && Minute == m_pNow->tm_min)
				{
					bell_alarm();
					Alarm = true;
					break;
				}
			}
		}
	}
	if (!Alarm && m_Chime)
	{
		if (m_pNow->tm_min == 0)
			bell_hour();
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
				SDL_Rect dest{(m_Width - w) / 2, *piY, w, h};
				SDL_RenderCopy(m_pRen, texture, nullptr, &dest);
				SDL_DestroyTexture(texture);
				SDL_FreeSurface(surface);
				*piY += h;
			}
		}
	}
}

void Clock::Bell(std::list<CHIME_INFO> chimes)
{
	SDL_LockAudioDevice(m_Audio);
	if (m_Dings.empty())
	{
		for (auto & chime : chimes)
			m_Dings.push_back({chime.delay, chime.volume, chime.pitch});
	}
	SDL_UnlockAudioDevice(m_Audio);
	SDL_PauseAudioDevice(m_Audio, 0);
}

void Clock::ColorUp()
{
	if (m_Color < 255)
		++m_Color;
	Redraw();
}

void Clock::ColorDown()
{
	if (m_Color > 0)
		--m_Color;
	Redraw();
}

void Clock::Silent()
{
	SDL_LockAudioDevice(m_Audio);
	m_Dings.remove_if([](auto & chime){return chime.waiting();});
	SDL_UnlockAudioDevice(m_Audio);
}

void Clock::ToggleAlarm()
{
	m_Alarm = !m_Alarm;
	Redraw();
}

void Clock::ToggleChime()
{
	m_Chime = !m_Chime;
	Redraw();
}

void Clock::RingBell()
{
	bell_test();
}

void Clock::PlayDing(unsigned char* pBuffer, int Length)
{
	SDL_memset(pBuffer, 0, Length);
	SDL_LockAudioDevice(m_Audio);
	if (m_Dings.empty())
		SDL_PauseAudioDevice(m_Audio, 1);
	else
	{
		for (auto it = m_Dings.begin(); it != m_Dings.end();)
		{
			if (it->play(reinterpret_cast<float*>(pBuffer)))
				++it;
			else
				it = m_Dings.erase(it);
		}
	}
	SDL_UnlockAudioDevice(m_Audio);
}

/*
void Clock::test()
{
	float buffer[SAMPLE_COUNT];
	float s_max = 0;
	m_pNow = new tm;
	for (int i = 0; i < 24; ++i)
	{
		for (int j = 0; j < 60; ++j)
		{
			m_pNow->tm_hour = i;
			m_pNow->tm_min = j;
			m_Tence = std::max(0, 6 * 60 - std::abs(m_pNow->tm_hour * 60 + m_pNow->tm_min - 12 * 60))
				/ 360.0f * (1.0f - TENCE_MIN) + TENCE_MIN;
			m_Pitch = 12 - std::abs(m_pNow->tm_hour - 12);
			bell_test();
			while (m_Dings.size())
			{
				PlayDing((unsigned char*)buffer, sizeof(buffer));
				for (auto & f : buffer)
				{
					s_max = std::max(s_max, f);
					s_max = std::max(s_max, -f);
				}
			}
			bell_alarm();
			while (m_Dings.size())
			{
				PlayDing((unsigned char*)buffer, sizeof(buffer));
				for (auto & f : buffer)
				{
					s_max = std::max(s_max, f);
					s_max = std::max(s_max, -f);
				}
			}
			if (m_pNow->tm_min == 0)
			{
				bell_hour();
				while (m_Dings.size())
				{
					PlayDing((unsigned char*)buffer, sizeof(buffer));
					for (auto & f : buffer)
					{
						s_max = std::max(s_max, f);
						s_max = std::max(s_max, -f);
					}
				}
			}
		}
	}
	delete m_pNow;
	if (s_max >= 1.0)
		throw "Max hit";
}
*/
