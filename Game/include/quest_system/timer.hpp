#pragma once
#include <chrono>

//chat gpt magic
namespace bee
{

class Timer
{
public:
    Timer() : m_startTimePoint(std::chrono::high_resolution_clock::now()), m_duration(0) {}

    void Start(std::chrono::milliseconds duration)
    {
        m_duration = duration;
        m_startTimePoint = std::chrono::high_resolution_clock::now();
    }

    bool IsTimeUp() const { return std::chrono::high_resolution_clock::now() - m_startTimePoint >= m_duration; }

private:
    std::chrono::high_resolution_clock::time_point m_startTimePoint;
    std::chrono::milliseconds m_duration;
};
}
