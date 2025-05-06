#include "actors/props/prop_template.hpp"

#include "magic_enum/magic_enum_all.hpp"

PropTemplate::PropTemplate() { InitializeAttributes(); }

void PropTemplate::InitializeAttributes()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.1);
}

void PropTemplate::AddAttribute(BaseAttributes attribute, double value)
{
    m_baseAttributes.insert(std::pair<BaseAttributes, double>(attribute, value));
}
