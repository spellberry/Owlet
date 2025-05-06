#include "user_interface/font_handler.hpp"

#include <fstream>
#include <thread>
#include <tinygltf/json.hpp>

#include "rendering/ui_renderer.hpp"
#include "tools/log.hpp"
#include "tools/serialization.hpp"
#include "tools/tools.hpp"
#include "user_interface/user_interface.hpp"

#ifdef BEE_PLATFORM_PC
#pragma warning(push, 0)
#include <msdf-atlas-gen/msdf-atlas-gen.h>
#pragma warning(pop)
#endif

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>

using namespace bee;
using namespace ui;

const Font& FontHandler::GetFont(const int id) const { return m_fonts.at(id); }
// unsigned int FontHandler::GetCodePoint(const char* letter, Font& font) { return font.codepointMap.find(*letter)->second; }

FontHandler ::~FontHandler()
{
    m_fonts.clear();
    ///    m_render.reset();
}

#ifdef BEE_PLATFORM_PC
bool FontHandler::SerializeFont(const std::string& fontLocation, const std::string& name, float fontSize) const
{
    bool success = false;
    if (msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype())
    {
        if (msdfgen::FontHandle* msdffont = msdfgen::loadFont(ft, fontLocation.c_str()))
        {
            Font font;

            std::vector<msdf_atlas::GlyphGeometry> glyphs;
            msdf_atlas::FontGeometry fontGeometry(&glyphs);

            fontGeometry.loadCharset(msdffont, 1.0, msdf_atlas::Charset::ASCII);
            const double maxCornerAngle = 3.0;
            for (msdf_atlas::GlyphGeometry& glyph : glyphs)
                glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

            msdf_atlas::TightAtlasPacker packer;

            packer.setDimensionsConstraint(msdf_atlas::TightAtlasPacker::DimensionsConstraint::SQUARE);
            packer.setMinimumScale(32.0f);

            const float pixelrange = 6.0f;
            packer.setPixelRange(pixelrange);
            packer.setMiterLimit(1.0);

            packer.pack(glyphs.data(), glyphs.size());

            int width = 0, height = 0;
            packer.getDimensions(width, height);

            msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator,
                                                msdf_atlas::BitmapAtlasStorage<msdf_atlas::byte, 3>>
                generator(width, height);
            // generator.resize(width, height);

            msdf_atlas::GeneratorAttributes attributes;

            generator.setAttributes(attributes);
            auto processor_count = std::thread::hardware_concurrency();
            if (processor_count == 0)
            {
                generator.setThreadCount(4);
            }
            else
            {
                generator.setThreadCount(processor_count);
            }

            generator.generate(glyphs.data(), glyphs.size());

            // general properties
            font.width = static_cast<float>(width);
            font.height = static_cast<float>(height);
            font.name = name;
            font.pixelrange = pixelrange;
            font.fontSize = fontSize;
            // linespace
            msdfgen::FontMetrics metrics;
            msdfgen::getFontMetrics(metrics, msdffont);
            font.lineSpace = static_cast<float>(metrics.ascenderY - metrics.descenderY + metrics.lineHeight) /
                             static_cast<float>(metrics.emSize);

            // individual glyph info
            double pl, pb, pr, pt;
            double il, ib, ir, it;
            for (auto& glpyh : glyphs)
            {
                unsigned int codepoint = glpyh.getCodepoint();
                font.advance.emplace(codepoint, glpyh.getAdvance());
                glpyh.getQuadPlaneBounds(pl, pb, pr, pt);
                glpyh.getQuadAtlasBounds(il, ib, ir, it);
                font.quadAtlasBounds.emplace(codepoint, glm::vec4(il, ib, ir, it));
                font.quadPlaneBounds.emplace(codepoint, glm::vec4(pl, pb, pr, pt));
            }

            // the texture
            msdfgen::BitmapConstRef<msdf_atlas::byte, 3> ref = generator.atlasStorage();
            int size = static_cast<int>(font.width * font.height) * 3;
            font.pepi.resize(size);
            for (int i = 0; i < size; i++)
            {
                font.pepi[i] = msdfgen::pixelByteToFloat(ref.pixels[i]);
            }

            // saving
            std::string str0 = "assets/fonts/" + name + ".sff";

            font.Save(str0);
        }
        success = true;
    }

    return success;
}
#endif

UIFontID FontHandler::LoadFont(const std::string& fontLocation)
{
    if (!fontLocation.find(".sff"))
    {
        Log::Warn("tried to load a non ssf file, for optimal performance please load a sff");
    }
    if (m_preCheckMap.count(fontLocation) > 0)
    {
        return m_preCheckMap.find(fontLocation)->second;
    }
    if (fileExists(fontLocation))
    {
        return Loadssf(fontLocation);
    }
    // no ssf file is there?
    std::string loc = fontLocation;
    RemoveSubstring(loc, ".sff");
    loc.append(".ttf");
    if (fileExists(loc))
    {
        std::string name = loc;
        RemoveSubstring(name, ".ttf");
        RemoveSubstring(name, "assets/fonts/");
        SerializeFont(loc, name);
        if (fileExists(fontLocation))
        {
            return Loadssf(fontLocation);
        }
    }
    RemoveSubstring(loc, ".ttf");
    loc.append(".otf");
    if (fileExists(loc))
    {
        std::string name = loc;
        RemoveSubstring(name, ".otf");
        RemoveSubstring(name, "assets/fonts/");
        SerializeFont(loc, name);
        if (fileExists(fontLocation))
        {
            return Loadssf(fontLocation);
        }
    }

    std::string str0 = "failed to load Font at full location: " + fontLocation;
    Log::Warn(str0);
    str0 = "No fallback otf or ttf were found ";
    Log::Warn(str0);

    return -1;
}

UIFontID FontHandler::Loadssf(const std::string& fontLocation)
{
    /*std::ifstream is(fontLocation);
    cereal::JSONInputArchive archive(is);*/

    Font font;
    /*archive(CEREAL_NVP(font));*/
    font.Load(fontLocation);

    auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();
    ui.m_renderer->GenFont(font, font.pepi);
    font.pepi.clear();
    font.id = m_fonts.size();
    m_fonts.push_back(font);
    m_preCheckMap.emplace(fontLocation, font.id);
    return m_fonts.size() - 1;
}

UIFontID bee::ui::FontHandler::LoadssfEditor(const std::string& fontLocation)
{
    // std::ifstream is(fontLocation);
    // cereal::JSONInputArchive archive(is);

    Font font;
    // archive(CEREAL_NVP(font));
    font.Load(fontLocation);

    auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();
    ui.m_renderer->GenFont(font, font.pepi);
    font.id = m_fonts.size();
    m_fonts.push_back(font);
    m_preCheckMap.emplace(fontLocation, font.id);
    return m_fonts.size() - 1;
}
