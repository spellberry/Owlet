#pragma once

#include "user_interface/user_interface_structs.hpp"

namespace bee
{
namespace ui
{

class FontHandler
{
public:
    ~FontHandler();

    bool SerializeFont(const std::string& fontLocation, const std::string& name, float fontSize = 2.0f) const;

private:
    friend class UserInterface;
    friend class UIEditor;

    const Font& GetFont(int id) const;
    UIFontID LoadFont(const std::string& fontLocation);
    UIFontID Loadssf(const std::string& fontLocation);
    UIFontID LoadssfEditor(const std::string& fontLocation);

    std::vector<Font> m_fonts;

    std::unordered_map<std::string, UIFontID> m_preCheckMap;
};
}  // namespace ui
}  // namespace bee