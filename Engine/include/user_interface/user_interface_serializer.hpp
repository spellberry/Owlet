#pragma once
#include <string>
#include <unordered_map>

#include "user_interface_editor_structs.hpp"
#include "user_interface_structs.hpp"

namespace bee
{
namespace ui
{

class UISerialiser
{
public:
    UISerialiser();
    void LoadOverlays();
    void LoadOverlays(UserInterface* ui);
    void SaveOverlays() const;
    ~UISerialiser();
    void LoadFunctions();
    void LoadFunctionsEditor();
    void SaveFunctions();

    UIElementID LoadElement(const std::string& name);
    void LoadIntoElement(UIElementID Element, const std::string& name);
    void SaveElement(const sElement& el) const;

private:
    friend class UIEditor;
    friend class UserInterface;

    void LoadSElement(UIElementID ID, sElement el);
    std::unordered_map<std::string, unsigned int> overlays;
    std::unordered_map<std::string, unsigned int> functions;
};

}  // namespace ui
}  // namespace bee