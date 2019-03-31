#include "chime.h"

#include <cmath>

const std::vector<std::pair<float, float>> chime::amplitude[FREQUENCY_COUNT] =  
{
    {
        {0.0f, 0.0f},
        {0.002f, 0.2f},
        {0.05f, 0.1f},
        {0.065f, 0.12f},
        {0.1f, 0.2f},
        {0.115f, 0.18f},
        {0.15f, 0.22f},
        {0.165f, 0.19f},
        {0.2f, 0.25f},
        {0.215f, 0.2f},
        {0.25f, 0.28f},
        {0.265f, 0.25f},
        {0.3f, 0.3f},
        {0.315f, 0.25f},
        {0.35f, 0.35f},
        {0.365f, 0.31f},
        {0.4f, 0.35f},
        {0.5f, 0.3f},
        {0.6f, 0.2f},
        {1.0f, 0.0f},
    },
    {
        {0.0f, 0.0f},
        {0.002f, 0.3f},
        {0.01f, 0.15f},
        {0.06f, 0.4f},
        {0.09f, 0.45f},
        {0.1f, 0.5f},
        {0.12f, 0.45f},
        {0.15f, 0.5f},
        {0.17f, 0.48f},
        {0.20f, 0.52f},
        {0.22f, 0.32f},
        {0.25f, 0.40f},
        {0.3f, 0.25f},
        {0.32f, 0.28f},
        {0.48f, 0.05f},
        {0.53f, 0.09f},
        {0.58f, 0.13f},
        {0.64f, 0.15f},
        {0.7f, 0.16f},
        {0.8f, 0.05f},
        {1.0f, 0.0f},
    },
    {
        {0.0f, 0.0f},
        {0.002f, 0.4f},
        {0.01f, 0.1f},
        {0.05f, 0.3f},
        {0.1f, 0.38f},
        {0.12f, 0.45f},
        {0.15f, 0.5f},
        {0.20f, 0.3f},
        {0.22f, 0.5f},
        {0.25f, 0.3f},
        {0.3f, 0.4f},
        {0.32f, 0.35f},
        {0.4f, 0.3f},
        {0.5f, 0.32f},
        {0.59f, 0.15f},
        {0.68f, 0.16f},
        {0.72f, 0.1f},
        {0.85f, 0.09f},
        {0.9f, 0.04f},
        {1.0f, 0.0f},
    },
    {
        {0.0f, 0.0f},
        {0.002f, 0.15f},
        {0.01f, 0.2f},
        {0.03f, 0.15f},
        {0.1f, 0.05f},
        {0.12f, 0.06f},
        {0.14f, 0.05f},
        {0.16f, 0.06f},
        {0.18f, 0.05f},
        {0.2f, 0.06f},
        {0.22f, 0.05f},
        {0.4f, 0.0f},
        {1.0f, 0.0f},
    },
    {
        {0.0f, 0.0f},
        {0.002f, 0.7f},
        {0.01f, 0.8f},
        {0.02f, 0.9f},
        {0.04f, 0.7f},
        {0.06f, 0.72f},
        {0.12f, 0.40f},
        {0.14f, 0.38f},
        {0.18f, 0.15f},
        {0.2f, 0.1f},
        {0.3f, 0.01f},
        {0.4f, 0.001f},
        {1.0f, 0.0f},
    },
    {
        {0.0f, 0.0f},
        {0.002f, 0.20f},
        {0.0125f, 0.1f},
        {0.025f, 0.15f},
        {0.0375f, 0.1f},
        {0.05f, 0.15f},
        {0.0625f, 0.1f},
        {0.075f, 0.18f},
        {0.0875f, 0.08f},
        {0.1, 0.12f},
        {0.1125f, 0.04f},
        {0.125, 0.12f},
        {0.1375f, 0.05f},
        {0.15, 0.14f},
        {0.1625f, 0.05f},
        {0.175, 0.1f},
        {0.1875f, 0.05f},
        {0.2f, 0.07f},
        {0.21f, 0.04f},
        {0.22f, 0.05f},
        {0.23f, 0.03f},
        {0.24f, 0.04f},
        {0.25f, 0.02f},
        {0.26f, 0.03f},
        {0.27f, 0.01f},
        {0.28f, 0.02f},
        {0.29f, 0.01f},
        {1.0f, 0.0f},
    },
    {
        {0.0f, 0.0f},
        {0.002f, 0.25f},
        {0.0125f, 0.15f},
        {0.025f, 0.2f},
        {0.0375f, 0.15f},
        {0.05f, 0.17f},
        {0.0625f, 0.1f},
        {0.075f, 0.12f},
        {0.08125f, 0.08f},
        {0.0875f, 0.12f},
        {0.1125f, 0.05f},
        {0.125, 0.03f},
        {0.1375f, 0.02f},
        {0.15, 0.01f},
        {0.1625f, 0.02f},
        {0.175, 0.01f},
        {0.1875f, 0.02f},
        {0.2f, 0.01f},
        {0.21f, 0.02f},
        {0.3f, 0.00f},
        {1.0f, 0.0f},
    },
};

chime::chime(int seconds, float _volume, int hour)
{
    pos = -seconds * SEGMENT_COUNT * SAMPLE_COUNT;
    volume = std::pow(_volume, 0.5f);
    float note = 400 + 40 * (12 - std::abs(hour - 12));
    frequency[0] = note * std::pow(2.0f, -24.0f / 12.0f);
    frequency[1] = note * std::pow(2.0f, -12.0f / 12.0f);
    frequency[2] = note * std::pow(2.0f, -9.0f / 12.0f);
    frequency[3] = note * std::pow(2.0f, -4.0f / 12.0f);
    frequency[4] = note * std::pow(2.0f, 0.0f / 12.0f);
    frequency[5] = note * std::pow(2.0f, 7.0f / 12.0f);
    frequency[6] = note * std::pow(2.0f, 12.0f / 12.0f);
    index[0] = 0;
    index[1] = 0;
    index[2] = 0;
    index[3] = 0;
    index[4] = 0;
    index[5] = 0;
    index[6] = 0;
    index[7] = 0;
}

chime::~chime()
{
}

bool chime::play(short* buffer)
{
    if (pos < 0)
        pos += SAMPLE_COUNT;
    else
    {
        for (int i = 0; i < SAMPLE_COUNT; ++i, ++pos)
        {
            float time = float(pos) / (DURATION * SEGMENT_COUNT * SAMPLE_COUNT);
            for (int j = 0; j < FREQUENCY_COUNT; j++)
            {
                if (time > amplitude[j][index[j] + 1].first)
                    ++index[j];
                buffer[i] += 16000.0f * volume *
                    (amplitude[j][index[j]].second + (amplitude[j][index[j] + 1].second - amplitude[j][index[j]].second) *
                    (time - amplitude[j][index[j]].first) / (amplitude[j][index[j] + 1].first - amplitude[j][index[j]].first)) *
                    std::sin(pos * 8.0f * std::atan(1.0f) * frequency[j] / (SEGMENT_COUNT * SAMPLE_COUNT));
            }
        }
        if (pos == DURATION * SEGMENT_COUNT * SAMPLE_COUNT)
            return false;
    }
    return true;
}

bool chime::waiting()
{
    return pos < 0;
}