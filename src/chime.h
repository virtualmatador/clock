#include <vector>

#define SAMPLE_COUNT 1500
#define SEGMENT_COUNT 32
#define FREQUENCY_COUNT 7
#define DURATION 4

class chime
{
private:
    std::vector<float> wave_;
    static const float cent_[FREQUENCY_COUNT];
    static const std::vector<std::pair<float, float>> amplitude_[FREQUENCY_COUNT];
public:
    chime(int pitch);
    ~chime();
    bool play(float volume, int & pos, float* buffer, int count);
};
