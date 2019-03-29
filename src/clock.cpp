#include "clock.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

#define TENCE_MIN 0.2
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
	, m_Tence{0}
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
		if (m_pNow->tm_hour < 6 || m_pNow->tm_hour >= 18)
			m_Tence = TENCE_MIN;
		else
			m_Tence = std::sin(8.0 * std::atan(1) * ((m_pNow->tm_hour - 9) * 60 + m_pNow->tm_min) / 12.0 / 60.0)
				* (1.0 - TENCE_MIN) / 2.0 + (1.0 - TENCE_MIN) / 2.0 + TENCE_MIN;
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
		SDL_Color color{m_Color * m_Tence, m_Color * m_Tence, m_Color * m_Tence};

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
						Bell(16, 3, TENCE_MIN, 1.0);
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
				Bell(Count, 2, m_Tence, m_Tence);
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

void Clock::Bell(int Count, int Interval, double VolumeMin, double VolumeMax)
{
	int Type = 12 * ((m_Tence - TENCE_MIN) / (1.0 - TENCE_MIN));
	SDL_LockAudioDevice(m_Audio);
	if (m_Dings.empty())
	{
		for (int i = 0; i < Count; ++i)
			m_Dings.push_back({-i * Interval * SEGMENT_COUNT * SAMPLE_COUNT, VolumeMin + (VolumeMax - VolumeMin) * i / Count, Type});
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
	m_Dings.remove_if([](auto & Chime){return Chime.Pos <= 0;});
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
	Bell(2, 3, m_Tence, m_Tence);
}

void Clock::PlayDing(unsigned char* pBuffer, int Length)
{
	static const double Frequency[12][5] =
	{
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
		{880.0,440.0,523.25,220.0,587.33},
	};
	static const int Amplitude[5][81] =
	{
		{0,5000,7500,9800,8000,4000,4200,3800,3400,3600,3200,2800,2500,2200,1900,1700,1500,1400,1300,1200,1100,1000,900,800,700,600,500,400,300,200,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,0},
		{0,3800,3400,2500,3000,4000,4500,5000,5300,5500,5400,5300,5400,5500,5600,5700,5800,5600,5400,5000,4500,4200,4500,5000,4000,3500,3600,3700,3600,3500,3400,3200,3300,3000,3100,2800,2900,2600,2700,2400,2500,2200,2300,2000,2100,1800,1900,1600,1700,1400,1500,1200,1300,1000,1100,1000,1000,900,900,800,800,700,700,600,600,500,500,400,400,300,300,200,200,100,100,100,100,100,100,100,0},
		{0,3000,1700,1400,1500,1600,2200,2300,2500,2700,2800,2900,3000,3100,3000,2900,2800,2700,2600,2700,2900,3000,3200,3400,3300,3100,3000,2800,2700,2500,2300,2400,2500,3000,3200,3300,3600,3500,3200,3000,2800,2600,2500,2700,2400,2500,2200,2300,2000,2100,1800,1900,1600,1700,1400,1500,1200,1300,1000,1000,900,900,800,800,700,700,600,600,500,500,400,400,300,300,200,200,100,100,100,100,0},
		{0,600,800,900,1000,1000,1100,1100,1200,1200,1300,1300,1400,1400,1500,1500,1600,1600,1700,1700,1800,1800,1900,1900,2000,2000,2100,2100,2200,2200,2300,2300,2400,2400,2500,2500,2400,2400,2500,2500,2400,2400,2400,2300,2300,2300,2200,2200,2200,2100,2100,2100,2000,2000,2000,1900,1900,1900,1800,1800,1800,1700,1700,1700,1600,1600,1600,1500,1500,1400,1300,1200,1100,1000,800,600,400,300,200,100,0},
		{0,1500,500,2000,1800,1600,1400,1300,1200,1100,1000,900,800,700,600,500,800,1000,800,700,600,900,700,900,800,700,600,500,400,600,800,700,600,500,400,300,200,100,200,100,200,100,200,100,200,100,200,100,200,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,0},
	};
	static const int AmplitudeCount = sizeof(*Amplitude) / sizeof(**Amplitude) - 1;
	static const int Duration = 4;

	SDL_memset(pBuffer, 0, Length);
	SDL_LockAudioDevice(m_Audio);
	if (m_Dings.empty())
		SDL_PauseAudioDevice(m_Audio, 1);
	else
	{
		for (auto it = m_Dings.begin(); it != m_Dings.end(); ++it)
		{
			if (it->Pos < 0)
				it->Pos += SAMPLE_COUNT;
			else
			{
				for (int i = 0; i < SAMPLE_COUNT; ++i)
				{
					int Low = it->Pos / (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount);
					double Mod = it->Pos % (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount);
					for (int j = 0; j < 5; j++)
					((short*)pBuffer)[i] += it->Volume *
						(Amplitude[j][Low] + Mod / (Duration * SEGMENT_COUNT * SAMPLE_COUNT / AmplitudeCount) * (Amplitude[j][Low + 1] - Amplitude[j][Low])) *
						std::sin(it->Pos * 8.0 * std::atan(1.0) * ((Frequency[it->Type][j] - it->Pos / 80000.0) / (SEGMENT_COUNT * SAMPLE_COUNT)));
					++it->Pos;
				}
				if (it->Pos == Duration * SEGMENT_COUNT * SAMPLE_COUNT)
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
