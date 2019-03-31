#include <vector>

#define SAMPLE_COUNT 1500
#define SEGMENT_COUNT 32
#define FREQUENCY_COUNT 7
#define DURATION 5

class chime
{
private:
	int pos;
	float volume;
	float frequency[FREQUENCY_COUNT];
    int index[FREQUENCY_COUNT];
    static const std::vector<std::pair<float, float>> amplitude[FREQUENCY_COUNT];
public:
    chime(int seconds, float _volume, int hour);
    ~chime();
    bool play(short* buffer);
    bool waiting();
};
