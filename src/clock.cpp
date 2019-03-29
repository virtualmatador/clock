#include "clock.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

#define SAMPLE_COUNT 1500
#define SEGMENT_COUNT 32

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
	, m_iWidth{0}
	, m_frameTime{std::chrono::nanoseconds(0)}
	, m_pNow{nullptr}
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
	m_FontTime = TTF_OpenFontRW(m_FontSource, false, m_iWidth / 4);
	if (!m_FontTime)
		throw "TTF_OpenFont";
	SDL_RWseek(m_FontSource, 0, RW_SEEK_SET);
	m_FontDate = TTF_OpenFontRW(m_FontSource, false, m_iWidth / 10);
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
    Alarm.freq = SEGMENT_COUNT * SAMPLE_COUNT;
    Alarm.format = AUDIO_S16SYS;
    Alarm.channels = 1;
    Alarm.samples = SAMPLE_COUNT;
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
		Redraw();
		CheckBell();
		tPre = t;
	}
}

void Clock::Redraw()
{
	int iY = 0;
	if (SDL_RenderClear(m_pRen) == 0)
	{
		unsigned char iColor;
		if (m_pNow->tm_hour < 6 || m_pNow->tm_hour >= 18)
			iColor = m_Color * 0.2;
		else
			iColor = m_Color * (std::sin(2.0 * std::atan(1) * 4.0 *
				((m_pNow->tm_hour - 9) * 60 + m_pNow->tm_min) / 12.0 / 60.0) * 0.8 / 2.0 + 0.8 / 2.0 + 0.2);
		SDL_Color color{iColor, iColor, iColor};

		std::stringstream sTime;
		sTime <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_hour << ":" <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_min << ":" <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_sec;
		DrawText(sTime.str(), m_FontTime, color, &iY);

		std::stringstream sDay;
		sDay << Clock::m_WeekDays[m_pNow->tm_wday];
		DrawText(sDay.str(), m_FontDate, color, &iY);

		std::stringstream sDate;
		sDate <<
			m_pNow->tm_year + 1900 << "/" <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_mon + 1 << "/" <<
			std::setfill('0') << std::setw(2) << m_pNow->tm_mday;
		DrawText(sDate.str(), m_FontDate, color, &iY);

		std::stringstream sInfo;
		sInfo << "[C]HIME:" << (m_Chime ? '\x7' : '\x8') << "  [A]LARM:" << (m_Alarm ? '\x7' : '\x8');
		DrawText(sInfo.str(), m_FontDate, color, &iY);
	}
	SDL_RenderPresent(m_pRen);
}

void Clock::CheckBell()
{
	static int MinPre = -1;
	if (MinPre != m_pNow->tm_min)
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
						Bell(24);
						Alarm = true;
						break;
					}
				}
			}
		}
		if (!Alarm && m_Chime)
		{
			if (m_pNow->tm_min == 0)
			{
				int Count = m_pNow->tm_hour % 12;
				if (Count == 0)
					Count = 12;
				Bell(Count);
			}
		}
		MinPre = m_pNow->tm_min;
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

void Clock::Bell(int Count)
{
	SDL_LockAudioDevice(m_Audio);
	if (m_Dings.empty())
	{
		for (int i = 0; i < Count; ++i)
			m_Dings.push_back(-i * 2 * SEGMENT_COUNT * SAMPLE_COUNT);
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
	m_Dings.remove_if([](auto & i){return i <= 0;});
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
	Bell(2);
}

void Clock::PlayDing(unsigned char* pBuffer, int Length)
{
	static const int Nominal[] =
	{
		0,500,750,980,800,400,420,380,340,360,320,280,250,220,190,170,150,140,130,120,110,100,90,80,70,60,50,40,30,20,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,0
	};
	static const int Prime[] =
	{
		0,380,340,250,300,400,450,500,530,550,540,530,540,550,560,570,580,560,540,500,450,420,450,500,400,350,360,370,360,350,340,320,330,300,310,280,290,260,270,240,250,220,230,200,210,180,190,160,170,140,150,120,130,100,110,100,100,90,90,80,80,70,70,60,60,50,50,40,40,30,30,20,20,10,10,10,10,10,10,10,0
	};
	static const int Tierce[] =
	{
		0,300,170,140,150,160,220,230,250,270,280,290,300,310,300,290,280,270,260,270,290,300,320,340,330,310,300,280,270,250,230,240,250,300,320,330,360,350,320,300,280,260,250,270,240,250,220,230,200,210,180,190,160,170,140,150,120,130,100,100,90,90,80,80,70,70,60,60,50,50,40,40,30,30,20,20,10,10,10,10,0
	};
	static const int Hum[] =
	{
		0,60,80,90,100,100,110,110,120,120,130,130,140,140,150,150,160,160,170,170,180,180,190,190,200,200,210,210,220,220,230,230,240,240,250,250,240,240,250,250,240,240,240,230,230,230,220,220,220,210,210,210,200,200,200,190,190,190,180,180,180,170,170,170,160,160,160,150,150,140,130,120,110,100,80,60,40,30,20,10,0
	};
	static const int Quint[] =
	{
		0,150,50,200,180,160,140,130,120,110,100,90,80,70,60,50,80,100,80,70,60,90,70,90,80,70,60,50,40,60,80,70,60,50,40,30,20,10,20,10,20,10,20,10,20,10,20,10,20,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,0
	};

	static const int AmplitudeCount = sizeof(Nominal) / sizeof(*Nominal) - 1;

	static const double NominalFrequency = 880.0;
	static const double PrimeFrequency = 440.0;
	static const double TierceFrequency = 523.25;
	static const double HumFrequency = 220.0;
	static const double QuintFrequency = 587.33;

	static const int Duration = 3;

	SDL_memset(pBuffer, 0, Length);
	SDL_LockAudioDevice(m_Audio);
	if (m_Dings.empty())
		SDL_PauseAudioDevice(m_Audio, 1);
	else
	{
		for (auto it = m_Dings.begin(); it != m_Dings.end(); ++it)
		{
			if (*it < 0)
				*it += SAMPLE_COUNT;
			else
			{
				for (int i = 0; i < SAMPLE_COUNT; ++i)
				{
					int Low = *it / (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount);
					double Mod = *it % (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount);
					((short*)pBuffer)[i] += 6 * (
						(Nominal[Low] + Mod / (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount) * (Nominal[Low + 1] - Nominal[Low])) *
						sin(*it * atan(1.0) * ((NominalFrequency - *it / 80000.0) * 8.0 / (SEGMENT_COUNT * SAMPLE_COUNT))) +
						(Prime[Low] + Mod / (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount) * (Prime[Low + 1] - Prime[Low])) *
						sin(*it * atan(1.0) * ((PrimeFrequency - *it / 30000.0) * 8.0 / (SEGMENT_COUNT * SAMPLE_COUNT))) +
						(Tierce[Low] + Mod / (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount) * (Tierce[Low + 1] - Tierce[Low])) *
						sin(*it * atan(1.0) * ((TierceFrequency - *it / 40000.0) * 8.0 / (SEGMENT_COUNT * SAMPLE_COUNT))) +
						(Hum[Low] + Mod / (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount) * (Hum[Low + 1] - Hum[Low])) *
						sin(*it * atan(1.0) * ((HumFrequency - *it / 60000.0) * 8.0 / (SEGMENT_COUNT * SAMPLE_COUNT))) +
						(Quint[Low] + Mod / (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount) * (Quint[Low + 1] - Quint[Low])) *
						sin(*it * atan(1.0) * ((QuintFrequency - *it / 50000.0) * 8.0 / (SEGMENT_COUNT * SAMPLE_COUNT))) +
						0);
					++*it;
				}
				if (*it == Duration * SEGMENT_COUNT * SAMPLE_COUNT)
				{
					auto tmp = std::prev(it);
					m_Dings.erase(it);
					it = tmp;
				}
			}
		}
	}
	SDL_UnlockAudioDevice(m_Audio);
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
