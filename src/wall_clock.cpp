#include "wall_clock.h"

#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

const char* wall_clock::weekdays_[] =
{
	"SUNDAY",
	"MONDAY",
	"TUESDAY",
	"WEDNESDAY",
	"THURSDAY",
	"FRIDAY",
	"SATURDAY",
};

void play_audio(void* pData, unsigned char* pBuffer, int Length)
{
	((wall_clock*)pData)->play_chimes(pBuffer, Length);
}

wall_clock::wall_clock()
	: wnd_{nullptr}
	, renderer_{nullptr}
	, shade_{255}
	, width_{0}
	, frame_time_{std::chrono::nanoseconds(0)}
	, now_{nullptr}
	, tence_{TENCE_MIN}
	, pitch_{0}
	, has_chime_{false}
	, has_alarm_{false}
	, audio_device_{0}
	, chimes_{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		throw "SDL_INIT";
	create_window();
	if (TTF_Init() < 0)
		throw "TTF_Init";
	font_source_ = SDL_RWFromConstMem(_binary_res_Font_ttf_start,
		_binary_res_Font_ttf_end - _binary_res_Font_ttf_start);	
	SDL_RWseek(font_source_, 0, RW_SEEK_SET);
	font_big_ = TTF_OpenFontRW(font_source_, false, width_ / 4);
	if (!font_big_)
		throw "TTF_OpenFont";
	SDL_RWseek(font_source_, 0, RW_SEEK_SET);
	font_medium_ = TTF_OpenFontRW(font_source_, false, width_ / 8);
	if (!font_medium_)
		throw "TTF_OpenFont";
	font_small_ = TTF_OpenFontRW(font_source_, false, width_ / 16);
	if (!font_small_)
		throw "TTF_OpenFont";
	create_audio();
}

wall_clock::~wall_clock()
{
	SDL_CloseAudioDevice(audio_device_);
	TTF_CloseFont(font_big_);
	TTF_CloseFont(font_medium_);
	TTF_CloseFont(font_small_);
	TTF_Quit();
	SDL_RWclose(font_source_);
	SDL_DestroyRenderer(renderer_);
	SDL_DestroyWindow(wnd_);
	SDL_Quit();
}

void wall_clock::create_window()
{
	SDL_ShowCursor(SDL_DISABLE);
	wnd_ = SDL_CreateWindow("wall_clock", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (!wnd_)
		throw "SDL_create_window";
	SDL_GetWindowSize(wnd_, &width_, nullptr);
	renderer_ = SDL_CreateRenderer(wnd_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer_)
		throw "SDL_CreateRenderer";
}

void wall_clock::create_audio()
{
	SDL_AudioSpec Alarm;
	SDL_zero(Alarm);
    Alarm.freq = SEGMENT_COUNT * SAMPLE_COUNT;
    Alarm.format = AUDIO_F32SYS;
    Alarm.channels = 1;
    Alarm.samples = SAMPLE_COUNT;
    Alarm.callback = play_audio;
    Alarm.userdata = this;
	audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &Alarm, nullptr, 0);
 	if (!audio_device_)
		throw "SDL_create_audio";
}

void wall_clock::run()
{
	for(;;)
	{
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		long tmp = now.time_since_epoch().count() % 1000000000;
		now -= std::chrono::nanoseconds(tmp);
		if (now != frame_time_)
		{
			frame_time_ = now;
			tick();
		}
		else
		{
			SDL_Event event;
			if (SDL_WaitEventTimeout(&event, (1000000000 - tmp) / 1000000) == 1)
			{
				if (handle_event(&event) < 0)
					break;
			}
		}
	}
}

int wall_clock::handle_event(SDL_Event* pEvent)
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
			shade_up();
			break;
		case SDL_SCANCODE_DOWN:
			shade_down();
			break;
		case SDL_SCANCODE_SPACE:
			silent();
			break;
		case SDL_SCANCODE_A:
			toggle_alarm();
			break;
		case SDL_SCANCODE_C:
			toggle_chime();
			break;
		case SDL_SCANCODE_R:
			test_bell();
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

void wall_clock::tick()
{
	static std::time_t tPre{0};
	std::time_t t = std::chrono::system_clock::to_time_t(frame_time_);
	if (tPre != t)
	{
		now_ = std::localtime(&t);
		static int MinPre = -1;
		if (MinPre != now_->tm_min)
		{
			tence_ = std::max(0, 6 * 60 - std::abs(now_->tm_hour * 60 + now_->tm_min - 12 * 60))
				/ 360.0f * (1.0f - TENCE_MIN) + TENCE_MIN;
			pitch_ = 12 - std::abs(now_->tm_hour - 12);
			check_bell();
			MinPre = now_->tm_min;
		}
		redraw();
		tPre = t;
	}
}

void wall_clock::redraw()
{
	int iY = 0;
	if (SDL_RenderClear(renderer_) == 0)
	{
		SDL_Color color
		{
			(unsigned char)(shade_ * tence_),
			(unsigned char)(shade_ * tence_),
			(unsigned char)(shade_ * tence_),
		};

		std::stringstream sTime;
		sTime <<
			std::setfill('0') << std::setw(2) << now_->tm_hour << ":" <<
			std::setfill('0') << std::setw(2) << now_->tm_min << ":" <<
			std::setfill('0') << std::setw(2) << now_->tm_sec;
		draw_text(sTime.str(), font_big_, color, &iY);

		std::stringstream sDay;
		sDay << wall_clock::weekdays_[now_->tm_wday];
		draw_text(sDay.str(), font_medium_, color, &iY);

		std::stringstream sDate;
		sDate <<
			now_->tm_year + 1900 << "/" <<
			std::setfill('0') << std::setw(2) << now_->tm_mon + 1 << "/" <<
			std::setfill('0') << std::setw(2) << now_->tm_mday;
		draw_text(sDate.str(), font_medium_, color, &iY);

		std::stringstream sInfo;
		sInfo << "\x5:" << (has_chime_ ? '\x7' : '\x8') << "  " << "\x6:" << (has_alarm_ ? '\x7' : '\x8');
		draw_text(sInfo.str(), font_small_, color, &iY);
	}
	SDL_RenderPresent(renderer_);
}

void wall_clock::bell_alarm()
{
	SDL_LockAudioDevice(audio_device_);
	if (strikes_.empty())
	{
		for (int i = 0; i < 13; ++i)
		{
			strikes_.push_back(
				{
					-int((i * 4.0f + 0.0f) * SEGMENT_COUNT) * SAMPLE_COUNT,
					tence_ * (i + 1) / 13.0f * 1.0f,
					i
				});
			strikes_.push_back(
				{
					-int((i * 4.0f + 1.0f) * SEGMENT_COUNT) * SAMPLE_COUNT,
					tence_ * (i + 1) / 13.0f * 0.5f,
					i
				});
		}
	}
	SDL_UnlockAudioDevice(audio_device_);
	SDL_PauseAudioDevice(audio_device_, 0);
}

void wall_clock::bell_hour()
{
	int count = now_->tm_hour % 12;
	if (count == 0)
		count = 12;
	SDL_LockAudioDevice(audio_device_);
	if (strikes_.empty())
	{
		for (int i = 0; i < count; ++i)
		{
			strikes_.push_back(
				{
					-int((i * 1.5f + 0.0f) * SEGMENT_COUNT) * SAMPLE_COUNT,
					tence_ * 0.75f,
					pitch_
				});
		}
	}
	SDL_UnlockAudioDevice(audio_device_);
	SDL_PauseAudioDevice(audio_device_, 0);
}

void wall_clock::bell_test()
{
	SDL_LockAudioDevice(audio_device_);
	if (strikes_.empty())
	{
		for (int i = 0; i < 2; ++i)
		{
			strikes_.push_back(
				{
					-int((i * 2.0f + 0.0f) * SEGMENT_COUNT) * SAMPLE_COUNT,
					tence_ * 0.75f,
					pitch_
				});
		}
	}
	SDL_UnlockAudioDevice(audio_device_);
	SDL_PauseAudioDevice(audio_device_, 0);
}

void wall_clock::check_bell()
{
	bool Alarm = false;
	if (has_alarm_)
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
				if (colon == ':' && Hour == now_->tm_hour && Minute == now_->tm_min)
				{
					bell_alarm();
					Alarm = true;
					break;
				}
			}
		}
	}
	if (!Alarm && has_chime_)
	{
		if (now_->tm_min == 0)
			bell_hour();
	}
}

void wall_clock::draw_text(const std::string & sText, TTF_Font* const pFont, const SDL_Color & color, int * piY)
{
	SDL_Surface* surface = TTF_RenderText_Solid(pFont, sText.c_str(), color);
	if (surface)
	{
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
		if (texture)
		{
			int w, h;
			if (SDL_QueryTexture(texture, nullptr, nullptr, &w, &h) == 0)
			{
				SDL_Rect dest{(width_ - w) / 2, *piY, w, h};
				SDL_RenderCopy(renderer_, texture, nullptr, &dest);
				SDL_DestroyTexture(texture);
				SDL_FreeSurface(surface);
				*piY += h;
			}
		}
	}
}

void wall_clock::shade_up()
{
	if (shade_ < 255)
		++shade_;
	redraw();
}

void wall_clock::shade_down()
{
	if (shade_ > 0)
		--shade_;
	redraw();
}

void wall_clock::silent()
{
	SDL_LockAudioDevice(audio_device_);
	strikes_.remove_if([](auto & strike){return strike.pos <= 0;});
	SDL_UnlockAudioDevice(audio_device_);
}

void wall_clock::toggle_alarm()
{
	has_alarm_ = !has_alarm_;
	redraw();
}

void wall_clock::toggle_chime()
{
	has_chime_ = !has_chime_;
	redraw();
}

void wall_clock::test_bell()
{
	bell_test();
}

void wall_clock::play_chimes(unsigned char* pBuffer, int Length)
{
	SDL_memset(pBuffer, 0, Length);
	SDL_LockAudioDevice(audio_device_);
	if (strikes_.empty())
		SDL_PauseAudioDevice(audio_device_, 1);
	else
	{
		for (auto it = strikes_.begin(); it != strikes_.end();)
		{
			if (chimes_[it->pitch].play(it->volume / 1.7f, it->pos, reinterpret_cast<float*>(pBuffer), Length / sizeof(float)))
				++it;
			else
				it = strikes_.erase(it);
		}
	}
	SDL_UnlockAudioDevice(audio_device_);
}

/*
void wall_clock::test()
{
	float buffer[SAMPLE_COUNT];
	now_ = new tm;
	for (int i = 0; i < 24; ++i)
	{
		for (int j = 0; j < 60; ++j)
		{
			float s_max = 0;
			now_->tm_hour = i;
			now_->tm_min = j;
			tence_ = std::max(0, 6 * 60 - std::abs(now_->tm_hour * 60 + now_->tm_min - 12 * 60))
				/ 360.0f * (1.0f - TENCE_MIN) + TENCE_MIN;
			pitch_ = 12 - std::abs(now_->tm_hour - 12);
			bell_test();
			while (strikes_.size())
			{
				play_chimes((unsigned char*)buffer, sizeof(buffer));
				for (auto & f : buffer)
				{
					s_max = std::max(s_max, f);
					s_max = std::max(s_max, -f);
				}
			}
			bell_alarm();
			while (strikes_.size())
			{
				play_chimes((unsigned char*)buffer, sizeof(buffer));
				for (auto & f : buffer)
				{
					s_max = std::max(s_max, f);
					s_max = std::max(s_max, -f);
				}
			}
			if (now_->tm_min == 0)
			{
				bell_hour();
				while (strikes_.size())
				{
					play_chimes((unsigned char*)buffer, sizeof(buffer));
					for (auto & f : buffer)
					{
						s_max = std::max(s_max, f);
						s_max = std::max(s_max, -f);
					}
				}
			}
			if (s_max >= 1.0)
				throw "Max hit";
			std::cout << i << ":" << j << " " << s_max << std::endl;
		}
	}
	delete now_;
}
*/
