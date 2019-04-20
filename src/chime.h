#include <vector>

#define SAMPLE_COUNT 1500
#define SEGMENT_COUNT 32
#define FREQUENCY_COUNT 7
#define DURATION 4

class chime
{
private:
	int pos;
    int pos_end[FREQUENCY_COUNT];
    int progress[FREQUENCY_COUNT];
    float base_amplitude[FREQUENCY_COUNT];
    float jump_amplitude[FREQUENCY_COUNT];
	float volume;
	float frequency[FREQUENCY_COUNT];
    static const float cent[FREQUENCY_COUNT];
    static const std::vector<std::pair<float, float>> amplitude[FREQUENCY_COUNT];
public:
    chime(float interval, float _volume, int pitch);
    ~chime();
    void set_step(int index);
    bool play(float* buffer, int count);
    bool waiting();
};
