#include "rendering/debug_render.hpp"
#include <glm/gtc/constants.hpp>

using namespace bee;
using namespace glm;

void DebugRenderer::AddLine(DebugCategory::Enum category, const vec2& from, const vec2& to, const vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    AddLine2D(category, vec2(from.x, from.y), vec2(to.x, to.y), color);
}

void DebugRenderer::AddCircle(DebugCategory::Enum category, const vec3& center, float radius, const vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    constexpr float dt = glm::two_pi<float>() / 32.0f;
    float t = 0.0f;

    vec3 v0(center.x + radius * cos(t), center.y + radius * sin(t), center.z);
    for (; t < glm::two_pi<float>(); t += dt)
    {
        vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
        AddLine(category, v0, v1, color);
        v0 = v1;
    }
}

void DebugRenderer::AddSphere(DebugCategory::Enum category, const glm::vec3& center, float radius, const glm::vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    constexpr float dt = glm::two_pi<float>() / 64.0f;
    float t = 0.0f;

    vec3 v0(center.x + radius * cos(t), center.y + radius * sin(t), center.z);
    for (; t < glm::two_pi<float>() - dt; t += dt)
    {
        vec3 v1(center.x + radius * cos(t + dt), center.y + radius * sin(t + dt), center.z);
        AddLine(category, v0, v1, color);
        v0 = v1;
    }
}

void DebugRenderer::AddSquare(DebugCategory::Enum category, const glm::vec3& center, float size, const glm::vec4& color)
{
    if (!(m_categoryFlags & category)) return;

    const float s = size * 0.5f;
    auto A = center + vec3(-s, -s, 0.0f);
    auto B = center + vec3(-s, s, 0.0f);
    auto C = center + vec3(s, s, 0.0f);
    auto D = center + vec3(s, -s, 0.0f);

    AddLine(category, A, B, color);
    AddLine(category, B, C, color);
    AddLine(category, C, D, color);
    AddLine(category, D, A, color);
}
