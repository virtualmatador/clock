#include "wall_clock.h"

#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdlib>

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
	, height_{0}
	, frame_time_{std::chrono::nanoseconds(0)}
	, now_{0}
	, tence_{TENCE_MIN}
	, pitch_{0}
	, has_chime_{false}
	, has_alarm_{false}
	, audio_device_{0}
	, texture_second_{nullptr}
	, texture_time_{nullptr}
	, texture_day_{nullptr}
	, texture_date_{nullptr}
	, texture_options_{nullptr}
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		throw "SDL_INIT";
	create_window();
	if (TTF_Init() < 0)
		throw "TTF_Init";
	font_source_ = SDL_RWFromConstMem(_binary_res_Font_ttf_start,
		_binary_res_Font_ttf_end - _binary_res_Font_ttf_start);
	int text_width = width_;
	do
	{
		SDL_RWseek(font_source_, 0, RW_SEEK_SET);
		font_big_ = TTF_OpenFontRW(font_source_, false, width_ * height_ * 4 / 9 / text_width);
		if (!font_big_)
			throw "TTF_OpenFont";
		if (TTF_SizeText(font_big_, "0", &digit_width_, nullptr) < 0 ||
			TTF_SizeText(font_big_, ":", &colon_width_, nullptr) < 0)
			throw "TTF_SizeText";
		text_width = digit_width_ * 6 + colon_width_ * 2;
	} while (text_width > width_);
	if (text_width < width_)
		text_width = width_;
	SDL_RWseek(font_source_, 0, RW_SEEK_SET);
	font_medium_ = TTF_OpenFontRW(font_source_, false, width_ * height_ * 2 / 9 / text_width);
	if (!font_medium_)
		throw "TTF_OpenFont";
	SDL_RWseek(font_source_, 0, RW_SEEK_SET);
	font_small_ = TTF_OpenFontRW(font_source_, false, width_ * height_ * 1 / 9 / text_width);
	if (!font_small_)
		throw "TTF_OpenFont";
	std::vector<int> sequence(13);
	std::iota(sequence.begin(), sequence.end(), 0);
	chimes_.insert(chimes_.end(), sequence.begin(), sequence.end());
	create_audio();
}

wall_clock::~wall_clock()
{
	SDL_CloseAudioDevice(audio_device_);
	TTF_CloseFont(font_big_);
	TTF_CloseFont(font_medium_);
	TTF_CloseFont(font_small_);
	SDL_DestroyTexture(texture_second_);
	SDL_DestroyTexture(texture_time_);
	SDL_DestroyTexture(texture_day_);
	SDL_DestroyTexture(texture_date_);
	SDL_DestroyTexture(texture_options_);
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
	renderer_ = SDL_CreateRenderer(wnd_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer_)
		throw "SDL_CreateRenderer";
	if (SDL_GetRendererOutputSize(renderer_, &width_, &height_) != 0)
		throw "SDL_GetRendererOutputSize";
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

int wall_clock::handle_event(SDL_Event* event)
{
	int iResult = 0;
	switch (event->type)
	{
	case SDL_QUIT:
		iResult = -1;
		break;
	case SDL_KEYDOWN:
		switch(event->key.keysym.scancode)
		{
		case SDL_SCANCODE_ESCAPE:
			if (event->key.repeat == 0)
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
	static std::time_t tPre = std::chrono::system_clock::to_time_t(frame_time_ - std::chrono::minutes(1));
	std::time_t t = std::chrono::system_clock::to_time_t(frame_time_);
	if (tPre != t)
	{
		now_ = *std::localtime(&t);
		auto pre = *std::localtime(&tPre);
		if (pre.tm_min != now_.tm_min)
		{
			tence_ = std::max(0, 6 * 60 - std::abs(now_.tm_hour * 60 + now_.tm_min - 12 * 60))
				/ 360.0f * (1.0f - TENCE_MIN) + TENCE_MIN;
			pitch_ = 12 - std::abs(now_.tm_hour - 12);
			check_bell();
			redraw(false);
		}
		else
		{
			redraw(true);
		}
		tPre = t;
	}
}

void wall_clock::redraw(const bool second_only)
{
	int iY = 0;
	if (SDL_RenderClear(renderer_) != 0)
		throw "SDL_RenderClear";
	SDL_Color color
	{
		(unsigned char)(shade_ * tence_),
		(unsigned char)(shade_ * tence_),
		(unsigned char)(shade_ * tence_),
	};

	std::stringstream sSecond;
	sSecond << std::setfill('0') << std::setw(2) << now_.tm_sec;
	draw_text(&texture_second_, sSecond.str(), font_big_, color);
	if (!second_only)
	{
		std::stringstream sTime;
		sTime <<
			std::setfill('0') << std::setw(2) << now_.tm_hour << ":" <<
			std::setfill('0') << std::setw(2) << now_.tm_min << ":";
		draw_text(&texture_time_, sTime.str(), font_big_, color);

		std::stringstream sDay;
		sDay << wall_clock::weekdays_[now_.tm_wday];
		draw_text(&texture_day_, sDay.str(), font_medium_, color);

		std::stringstream sDate;
		sDate <<
			now_.tm_year + 1900 << "/" <<
			std::setfill('0') << std::setw(2) << now_.tm_mon + 1 << "/" <<
			std::setfill('0') << std::setw(2) << now_.tm_mday;
		draw_text(&texture_date_, sDate.str(), font_medium_, color);

		std::stringstream sInfo;
		sInfo << "\x5:" << (has_chime_ ? '\x7' : '\x8') << "  " << "\x6:" << (has_alarm_ ? '\x7' : '\x8');
		draw_text(&texture_options_, sInfo.str(), font_small_, color);
	}
	render_texture(texture_second_, width_ / 2 + digit_width_ * 2 + colon_width_, iY);
	iY += render_texture(texture_time_, width_ / 2 - digit_width_, iY);
	iY += render_texture(texture_day_, width_ / 2, iY);
	iY += render_texture(texture_date_, width_ / 2, iY);
	iY += render_texture(texture_options_, width_ / 2, iY);
	SDL_RenderPresent(renderer_);
}

void wall_clock::draw_text(SDL_Texture** texture, const std::string & text, TTF_Font* font, const SDL_Color & color)
{
	SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
	if (!surface)
		throw "TTF_RenderText_Solid";
	SDL_DestroyTexture(*texture);
	*texture = SDL_CreateTextureFromSurface(renderer_, surface);
	if (!*texture)
		throw "SDL_CreateTextureFromSurface";
	SDL_FreeSurface(surface);
}

int wall_clock::render_texture(SDL_Texture* texture, const int x, const int y)
{
	int w, h;
	if (SDL_QueryTexture(texture, nullptr, nullptr, &w, &h) != 0)
		throw "SDL_QueryTexture";
	SDL_Rect dest{x - w / 2, y, w, h};
	if (SDL_RenderCopy(renderer_, texture, nullptr, &dest) != 0)
		throw "SDL_RenderCopy";
	return h;
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
	int count = now_.tm_hour % 12;
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
	bool alarm = false;
	if (has_alarm_)
	{
		const char* home_directory = getenv("HOME");
		if (home_directory)
		{
			std::string conf_path(home_directory);
			conf_path += "/.clock.conf";
			std::ifstream settings(conf_path.c_str());
			std::string alarm_time;
			while (std::getline(settings, alarm_time))
			{
				std::istringstream iss(alarm_time);
				char colon;
				int hour, minute;
				if (iss >> hour >> colon >> minute)
				{
					if (colon == ':' && hour == now_.tm_hour && minute == now_.tm_min)
					{
						bell_alarm();
						alarm = true;
						break;
					}
				}
			}
		}
	}
	if (!alarm && has_chime_)
	{
		if (now_.tm_min == 0)
			bell_hour();
	}
}

void wall_clock::shade_up()
{
	if (shade_ < 255)
		++shade_;
	redraw(false);
}

void wall_clock::shade_down()
{
	if (shade_ > 0)
		--shade_;
	redraw(false);
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
	redraw(false);
}

void wall_clock::toggle_chime()
{
	has_chime_ = !has_chime_;
	redraw(false);
}

void wall_clock::test_bell()
{
	bell_test();
}

void wall_clock::play_chimes(unsigned char* buffer, int length)
{
	SDL_memset(buffer, 0, length);
	SDL_LockAudioDevice(audio_device_);
	if (strikes_.empty())
		SDL_PauseAudioDevice(audio_device_, 1);
	else
	{
		for (auto it = strikes_.begin(); it != strikes_.end();)
		{
			if (chimes_[it->pitch].play(it->volume / 1.7f, it->pos, reinterpret_cast<float*>(buffer), length / sizeof(float)))
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
			now_.tm_hour = i;
			now_.tm_min = j;
			tence_ = std::max(0, 6 * 60 - std::abs(now_.tm_hour * 60 + now_.tm_min - 12 * 60))
				/ 360.0f * (1.0f - TENCE_MIN) + TENCE_MIN;
			pitch_ = 12 - std::abs(now_.tm_hour - 12);
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
			if (now_.tm_min == 0)
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
