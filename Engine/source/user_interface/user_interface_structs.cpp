#include "user_interface/user_interface_structs.hpp"

#include <cereal/archives/json.hpp>

#include "core/engine.hpp"
#include "tools/tools.hpp"

void bee::ui::Font::Save(const std::string& path)
{
    std::ofstream os(path);
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(width), CEREAL_NVP(lineSpace), CEREAL_NVP(height), CEREAL_NVP(pixelrange), CEREAL_NVP(fontSize),
            CEREAL_NVP(name), CEREAL_NVP(advance), CEREAL_NVP(quadPlaneBounds), CEREAL_NVP(quadAtlasBounds), CEREAL_NVP(pepi));
}

void bee::ui::Font::Load(const std::string& path)
{
    if (fileExists(path))
    {
        std::ifstream is(path);
        cereal::JSONInputArchive archive(is);

        archive(CEREAL_NVP(width), CEREAL_NVP(lineSpace), CEREAL_NVP(height), CEREAL_NVP(pixelrange), CEREAL_NVP(fontSize),
                CEREAL_NVP(name), CEREAL_NVP(advance), CEREAL_NVP(quadPlaneBounds), CEREAL_NVP(quadAtlasBounds),
                CEREAL_NVP(pepi));
    }
}