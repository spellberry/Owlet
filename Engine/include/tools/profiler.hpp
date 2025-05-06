#pragma once

#include <unordered_map>
#include <string>
#include <deque>
#include <chrono>

#if defined(BEE_PLATFORM_PC) && defined(BEE_PROFILE)
	#include <Superluminal/PerformanceAPI.h>
#else
	#define PERFORMANCEAPI_ENABLED 0
	#include <Superluminal/PerformanceAPI.h>
#endif


#ifndef GAME
#define BEE_PROFILE_FUNCTION() bee::ProfilerSection s_sect(__FUNCTION__); PERFORMANCEAPI_INSTRUMENT_FUNCTION()
#define BEE_PROFILE_SECTION(id) bee::ProfilerSection s_sect(id); PERFORMANCEAPI_INSTRUMENT(id)
#endif

#if defined (BEE_PLATFORM_PC)
using clock_type = std::chrono::steady_clock;
#else
using clock_type = std::chrono::system_clock;
#endif


namespace bee
{

using TimeT = std::chrono::time_point<clock_type>;
using SpanT = std::chrono::nanoseconds;

class ProfilerSection
{
public:
	ProfilerSection(const std::string &name);
	~ProfilerSection();
private:
	std::string m_name;
};

class Profiler
{
public:
	Profiler();
	~Profiler();
	void BeginSection(const std::string& name);
	void EndSection(const std::string& name);
	void Inspect();

private:
	struct Entry
	{
		TimeT Start {};
		TimeT End {};
		SpanT Accum{};
		float Avg = 0.0f;
		std::deque<float> History;
	};
	std::unordered_map<std::string, Entry> m_times;
};

}
