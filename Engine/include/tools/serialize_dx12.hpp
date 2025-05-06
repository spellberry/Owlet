#pragma once
#include "cereal/cereal.hpp"
#include <DirectXMath.h>

namespace DirectX
{
    template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT2& v) { archive(v.x, v.y); }
    template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT3& v) { archive(v.x, v.y, v.z); }
    template<class Archive> void serialize(Archive& archive, DirectX::XMFLOAT4& v) { archive(v.x, v.y, v.z, v.w); }
}
