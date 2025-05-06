#include "tools/profiler.hpp"

#if defined(BEE_PROFILE)
#if defined(BEE_PLATFORM_PC)
#include <Windows.h>
#include <Superluminal/PerformanceAPI_loader.h>
#endif
#include "tools/log.hpp"
#include "core/engine.hpp"
#include <imgui/imgui.h>
#include <imgui/implot.h>

using namespace bee;

ProfilerSection::ProfilerSection(const std::string& name) : m_name(name)
{
    Engine.Profiler().BeginSection(m_name);
}

ProfilerSection::~ProfilerSection()
{
    Engine.Profiler().EndSection(m_name);
}

Profiler::Profiler()
{
#if defined(PLATFORM_PC)
    PerformanceAPI_Functions performanceAPI;
    if (!PerformanceAPI_LoadFrom(L"PerformanceAPI.dll", &performanceAPI))
        log::warn("Superluminal PerformanceAPI could not be loaded");
    PerformanceAPI_SetCurrentThreadName("Main");
#endif
}

bee::Profiler::~Profiler() {}

void Profiler::BeginSection(const std::string& name)
{
    m_times[name].Start = std::chrono::high_resolution_clock::now();
}

void Profiler::EndSection(const std::string& name)
{   
    auto& e = m_times[name];
    e.End = std::chrono::high_resolution_clock::now();
    auto elapsed = e.End - e.Start;
    e.Accum += elapsed;
}

void Profiler::Inspect()
{
    ImGui::Begin("Profiler");

    for (auto& itr : m_times)
    {
        auto& e = itr.second;
        float duration = (float)((double)e.Accum.count() / 1000000.0);
        if (e.History.size() > 100)
            e.History.pop_front();
        e.History.push_back(duration);

        e.Avg = 0.0f;
        for (float f : e.History)
            e.Avg += f;

        e.Avg /= (float)e.History.size();
    }

    if (ImPlot::BeginPlot("Profiler"))
    {
        ImPlot::SetupAxes("Sample", "Time");
        ImPlot::SetupAxesLimits(0, 100, 0, 20);
        for (auto& itr : m_times)
        {
            auto& e = itr.second;

            std::vector<float> vals(
                e.History.begin(),
                e.History.end());            

            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded(itr.first.c_str(), vals.data(), (int)vals.size());
            ImPlot::PopStyleVar();
            ImPlot::PlotLine(itr.first.c_str(), vals.data(), (int)vals.size());
        }
        ImPlot::EndPlot();
    }

    for (auto& itr : m_times)
        ImGui::LabelText(itr.first.c_str(),"%f ms", itr.second.Avg);

    ImGui::End();   

    for (auto& itr : m_times)
        itr.second.Accum = {};
}

#else

bee::ProfilerSection::ProfilerSection(const std::string& name) {}
bee::ProfilerSection::~ProfilerSection() {}

bee::Profiler::Profiler() {}
bee::Profiler::~Profiler() {}
void bee::Profiler::Inspect() {}

#endif
