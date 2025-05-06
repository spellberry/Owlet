#pragma once
#include "cereal/cereal.hpp"
#include "imgui/imgui.h"

template<class Archive> void serialize(Archive& archive, ImVec2& v) { archive(v.x, v.y); }
template<class Archive> void serialize(Archive& archive, ImVec4& v) { archive(v.x, v.y, v.z, v.w); }
