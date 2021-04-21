#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>

#include <strings.h>

#include "wall_clock.h"

std::vector<const char*> wall_clock::weekdays_ =
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
	, font_source_{nullptr}
	, font_big_{nullptr}
	, font_medium_{nullptr}
	, font_small_{nullptr}
	, volume_{100}
	, brightness_{255}
	, display_{0}
	, width_{0}
	, height_{0}
	, frame_time_{std::chrono::nanoseconds(0)}
	, now_{0}
	, tence_{0}
	, pitch_{0}
	, has_chime_{false}
	, has_alarm_{false}
	, next_alarm_{std::size_t(-1)}
	, audio_device_{0}
	, texture_second_{nullptr}
	, texture_time_{nullptr}
	, texture_day_{nullptr}
	, texture_date_{nullptr}
	, texture_options_{nullptr}
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		throw "SDL_INIT";
	SDL_ShowCursor(SDL_DISABLE);
	wnd_ = SDL_CreateWindow("wall_clock", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		0, 0, SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS);
	if (!wnd_)
	{
		throw "SDL_create_window";
	}
	if (TTF_Init() < 0)
	{
		throw "TTF_Init";
	}
	font_source_ = SDL_RWFromConstMem(_binary_Font_ttf_start,
		_binary_Font_ttf_end - _binary_Font_ttf_start);
	if (!font_source_)
	{
		throw "SDL_RWFromConstMem";
	}
	set_window();
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

void wall_clock::set_window()
{
	SDL_Rect frame;
	if (SDL_GetDisplayBounds(display_, &frame) != 0)
	{
		throw "SDL_GetDisplayBounds";
	}
	SDL_DestroyRenderer(renderer_);
	SDL_HideWindow(wnd_);
	if (SDL_SetWindowFullscreen(wnd_, 0) != 0)
	{
		throw "SDL_SetWindowFullscreen";
	}	
	SDL_SetWindowPosition(wnd_, frame.x, frame.y);
	SDL_SetWindowSize(wnd_, frame.w, frame.h);
	if (SDL_SetWindowFullscreen(wnd_, SDL_WINDOW_FULLSCREEN) != 0)
	{
		throw "SDL_SetWindowFullscreen";
	}	
	SDL_ShowWindow(wnd_);
	renderer_ = SDL_CreateRenderer(wnd_, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer_)
	{
		throw "SDL_CreateRenderer";
	}
	if (SDL_GetRendererOutputSize(renderer_, &width_, &height_) != 0)
	{
		throw "SDL_GetRendererOutputSize";
	}
	int text_width = width_;
	do
	{
		SDL_RWseek(font_source_, 0, RW_SEEK_SET);
		TTF_CloseFont(font_big_);
		font_big_ = TTF_OpenFontRW(font_source_, false, width_ * height_ * 4 / 9 / text_width);
		if (!font_big_)
		{
			throw "TTF_OpenFont";
		}
		if (TTF_SizeText(font_big_, "0", &digit_width_, nullptr) < 0 ||
			TTF_SizeText(font_big_, ":", &colon_width_, nullptr) < 0)
		{
			throw "TTF_SizeText";
		}
		text_width = digit_width_ * 6 + colon_width_ * 2;
	} while (text_width > width_);
	if (text_width < width_)
	{
		text_width = width_;
	}
	SDL_RWseek(font_source_, 0, RW_SEEK_SET);
	TTF_CloseFont(font_medium_);
	font_medium_ = TTF_OpenFontRW(font_source_, false, width_ * height_ * 2 / 9 / text_width);
	if (!font_medium_)
	{
		throw "TTF_OpenFont";
	}
	SDL_RWseek(font_source_, 0, RW_SEEK_SET);
	TTF_CloseFont(font_small_);
	font_small_ = TTF_OpenFontRW(font_source_, false, width_ * height_ * 1 / 9 / text_width);
	if (!font_small_)
	{
		throw "TTF_OpenFont";
	}
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
	{
		throw "SDL_create_audio";
	}
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
				{
					break;
				}
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
	case SDL_KEYUP:
		switch(event->key.keysym.scancode)
		{
		case SDL_SCANCODE_ESCAPE:
			iResult = -1;
			break;
		case SDL_SCANCODE_SPACE:
			silent();
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

void wall_clock::read_config()
{
	volume_ = 100;
	brightness_ = 255;
	has_chime_ = false;
	has_alarm_ = false;
	next_alarm_ = std::size_t(-1);
	const char* home_directory = getenv("HOME");
	if (home_directory)
	{
		std::string conf_path { home_directory };
		conf_path += "/.clock.conf";
		std::ifstream config_file(conf_path.c_str());
		std::string config;
		std::set<std::size_t> alarms;
		while (std::getline(config_file, config))
		{
			std::istringstream pair_stream { config };
			std::string key;
			pair_stream >> key;
			if (key == "alarm")
			{
				char colon;
				int hour, minute;
				if (pair_stream >> hour >> colon >> minute)
				{
					if (colon == ':' && hour < 24 && hour >= 0 &&
						minute < 60 && minute >= 0)
					{
						std::vector<std::size_t> alarm_weekdays;
						std::string weekday;
						while (pair_stream >> weekday)
						{
							if (strcasecmp(weekday.c_str(), "weekdays") == 0)
							{
								for (const auto wd : {1, 2, 3, 4, 5})
								{
									alarm_weekdays.push_back(wd);
								}
							}
							else if (strcasecmp(weekday.c_str(), "weekend") == 0)
							{
								for (const auto wd : {0, 6})
								{
									alarm_weekdays.push_back(wd);
								}
							}
							else
							{
								auto day = std::find_if(weekdays_.begin(), weekdays_.end(),
									[&](const char* wd)
								{
									return strcasecmp(wd, weekday.c_str()) == 0;
								});
								if (day != weekdays_.end())
								{
									alarm_weekdays.push_back(day - weekdays_.begin());
								}
							}
						}
						if (alarm_weekdays.empty())
						{
							for (auto it = weekdays_.begin(); it != weekdays_.end(); ++it)
							{
								alarm_weekdays.push_back(it - weekdays_.begin());
							}
						}
						for (const auto& alarm_weekday : alarm_weekdays)
						{
							alarms.insert((alarm_weekday * 24 + hour) * 60 + minute);
						}
					}
				}
			}
			else if (key == "volume")
			{
				int volume;
				if (pair_stream >> volume && volume >= 0 && volume <= 100)
				{
					volume_ = volume;
				}
			}
			else if (key == "brightness")
			{
				int brightness;
				if (pair_stream >> brightness && brightness >= 0 && brightness < 256)
				{
					brightness_ = brightness;
				}
			}
			else if (key == "display")
			{
				int display;
				if (pair_stream >> display && display > 0 && display <= SDL_GetNumVideoDisplays())
				{
					display_ =  display - 1;
				}
			}
			else if (key == "chimes")
			{
				std::string chimes;
				if (pair_stream >> chimes)
				{
					if (chimes == "true")
					{
						has_chime_ = true;
					}
					else if (chimes == "false")
					{
						has_chime_ = false;
					}
				}
			}
			else if (key == "alarms")
			{
				std::string alarms;
				if (pair_stream >> alarms)
				{
					if (alarms == "true")
					{
						has_alarm_ = true;
					}
					else if (alarms == "false")
					{
						has_alarm_ = false;
					}
				}
			}
		}
		bool will_alarm = false;
		std::size_t alarm_time = (now_.tm_wday * 24 + now_.tm_hour) * 60 + now_.tm_min;
		auto alarm = alarms.lower_bound(alarm_time);
		if (alarm != alarms.end() && *alarm == alarm_time)
		{
			if (has_alarm_)
			{
				bell_alarm();
				will_alarm = true;
			}
			++alarm;
		}
		if (has_chime_)
		{
			if (now_.tm_min == 0 && !will_alarm)
			{
				bell_chime();
			}
		}
		if (alarm == alarms.end())
		{
			alarm = alarms.begin();
		}
		if (alarm != alarms.end())
		{
			next_alarm_ = *alarm;
		}
	}
}

float wall_clock::get_volume()
{
	return (volume_ / 100.0f) * tence_ * 0.5f;
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
				/ 360.0f * 0.75f + 0.25f;
			pitch_ = 12 - std::abs(now_.tm_hour - 12);
			read_config();
			if (SDL_GetWindowDisplayIndex(wnd_) != display_)
			{
				set_window();
			}
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
	{
		throw "SDL_RenderClear";
	}
	SDL_Color color
	{
		(unsigned char)(brightness_ * tence_),
		(unsigned char)(brightness_ * tence_),
		(unsigned char)(brightness_ * tence_),
	};

	std::stringstream sSecond;
	sSecond << std::setfill('0') << std::setw(2) << now_.tm_sec;
	draw_text(&texture_second_, sSecond.str(), font_big_, color);
	if (!second_only)
	{
		std::stringstream sTime;
		sTime << std::setfill('0') <<
			std::setw(2) << now_.tm_hour << ":" <<
			std::setw(2) << now_.tm_min << ":";
		draw_text(&texture_time_, sTime.str(), font_big_, color);

		std::stringstream sDay;
		sDay << wall_clock::weekdays_[now_.tm_wday];
		draw_text(&texture_day_, sDay.str(), font_medium_, color);

		std::stringstream sDate;
		sDate << std::setfill('0') <<
			std::setw(2) << now_.tm_mon + 1 << "/" <<
			std::setw(2) << now_.tm_mday <<	"/" <<
			now_.tm_year + 1900;
		draw_text(&texture_date_, sDate.str(), font_medium_, color);

		std::stringstream sInfo;
		sInfo
			<< "\x5:" << (has_chime_ ? '\x7' : '\x8') << "  "
			<< "\x6:" << (has_alarm_ ? '\x7' : '\x8') << " ";
		if (next_alarm_ != std::size_t(-1))
		{
			sInfo <<
				weekdays_[next_alarm_ / (60 * 24)][0] <<
				weekdays_[next_alarm_ / (60 * 24)][1] <<
				weekdays_[next_alarm_ / (60 * 24)][2] <<
				" " << std::setfill('0') <<
				std::setw(2) << next_alarm_ / 60 % 24 << ':' <<
				std::setw(2) << next_alarm_ % 60;
		}
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
	{
		throw "TTF_RenderText_Solid";
	}
	SDL_DestroyTexture(*texture);
	*texture = SDL_CreateTextureFromSurface(renderer_, surface);
	if (!*texture)
	{
		throw "SDL_CreateTextureFromSurface";
	}
	SDL_FreeSurface(surface);
}

int wall_clock::render_texture(SDL_Texture* texture, const int x, const int y)
{
	int w, h;
	if (SDL_QueryTexture(texture, nullptr, nullptr, &w, &h) != 0)
	{
		throw "SDL_QueryTexture";
	}
	SDL_Rect dest{x - w / 2, y, w, h};
	if (SDL_RenderCopy(renderer_, texture, nullptr, &dest) != 0)
	{
		throw "SDL_RenderCopy";
	}
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
				get_volume() * (i + 1) / 13.0f,
				i
			});
			strikes_.push_back(
			{
				-int((i * 4.0f + 1.0f) * SEGMENT_COUNT) * SAMPLE_COUNT,
				get_volume() * (i + 1) / 26.0f,
				i
			});
		}
	}
	SDL_UnlockAudioDevice(audio_device_);
	SDL_PauseAudioDevice(audio_device_, 0);
}

void wall_clock::bell_chime()
{
	int count = now_.tm_hour % 12;
	if (count == 0)
	{
		count = 12;
	}
	SDL_LockAudioDevice(audio_device_);
	if (strikes_.empty())
	{
		for (int i = 0; i < count; ++i)
		{
			strikes_.push_back(
			{
				-int((i * 1.5f + 0.0f) * SEGMENT_COUNT) * SAMPLE_COUNT,
				get_volume(), pitch_
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
				get_volume(), pitch_
			});
		}
	}
	SDL_UnlockAudioDevice(audio_device_);
	SDL_PauseAudioDevice(audio_device_, 0);
}

void wall_clock::silent()
{
	SDL_LockAudioDevice(audio_device_);
	strikes_.remove_if([](auto & strike){return strike.pos <= 0;});
	SDL_UnlockAudioDevice(audio_device_);
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
			{
				++it;
			}
			else
			{
				it = strikes_.erase(it);
			}
		}
	}
	SDL_UnlockAudioDevice(audio_device_);
}
