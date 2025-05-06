#include "actors/attributes.hpp"

#include <algorithm>
#include <optional>

#include "magic_enum/magic_enum.hpp"
#include "tools/log.hpp"

StatModifier::StatModifier() = default;

StatModifier::StatModifier(const ModifierType type, const double value) : m_modifierType(type), m_value(value){}

StatModifier::StatModifier(ModifierType type, double value, bool buff): m_modifierType(type), m_value(value), isBuff(buff){};

double StatModifier::GetModifiedValue(const double valueToModify) const
{
    double toReturn = valueToModify;
    if (m_modifierType == ModifierType::Additive)
    {
        toReturn += m_value;
    }
    else if (m_modifierType == ModifierType::Multiplier)
    {
        toReturn *= m_value;
    }

    return toReturn;
}

void StatModifier::SetValue(const double value) { m_value = value; }

double StatModifier::GetValue() const { return m_value; }

void StatModifier::SetModifierType(const ModifierType type) { m_modifierType = type; }

ModifierType StatModifier::GetModifierType() const { return m_modifierType; }

Attribute::Attribute(const double value) : m_baseValue(value) {}

Attribute::Attribute() {}

void Attribute::SetBaseValue(const double toSet) { m_baseValue = toSet; }

void Attribute::AddModifier(const StatModifier modifier) { m_modifiers.push_back(modifier); }
void Attribute::RemoveModifier(const StatModifier modifier)
{
    const auto it = std::find(m_modifiers.begin(), m_modifiers.end(), modifier);
    if (it != m_modifiers.end())
    {
        m_modifiers.erase(it);
    }
}

void Attribute::ClearModifiers() { m_modifiers.clear(); }

void Attribute::SetModifierValue(const StatModifier modifier, const double value)
{
    const auto l_modifier = std::find(m_modifiers.begin(), m_modifiers.end(), modifier);
    if (l_modifier != m_modifiers.end()) l_modifier->SetValue(value);
}
std::vector<StatModifier>& Attribute::GetModifiers() { return m_modifiers; }
std::vector<std::reference_wrapper<StatModifier>> Attribute::GetModifiersOfType(const ModifierType modType)
{
    std::vector<std::reference_wrapper<StatModifier>> filtered;
    for (auto& modifier : m_modifiers)
    {
        if (modifier.GetModifierType() == modType)
        {
            filtered.emplace_back(modifier);
        }
    }
    return filtered;
}

double Attribute::GetValue() const
{
    double toReturn = m_baseValue;
    auto tempModifiers = m_modifiers;
    if (!tempModifiers.empty())
    {
        for (auto& modifier : m_modifiers)
        {
            toReturn = modifier.GetModifiedValue(toReturn);
        }
    }

    return toReturn;
}

double Attribute::GetBaseValue() const { return m_baseValue; }

void AttributesComponent::SetAttributes(const std::unordered_map<BaseAttributes, Attribute>& attributes)
{
    m_attributes = attributes;
}

const std::unordered_map<BaseAttributes, Attribute>& AttributesComponent::GetAttributes() { return m_attributes; }

void AttributesComponent::SetAttribute(const BaseAttributes type, const double value)
{
    m_attributes[type].SetBaseValue(value);
}

double AttributesComponent::GetValue(const BaseAttributes type) const
{
    if (m_attributes.find(type) != m_attributes.end())
        return m_attributes.at(type).GetValue();
    else
        bee::Log::Warn("There is no {} in the {} template", magic_enum::enum_name(type), m_entityType);
    return 0;
}

void AttributesComponent::AddModifier(const BaseAttributes type, const StatModifier modifier)
{
    if (m_attributes.find(type) != m_attributes.end())
        m_attributes[type].AddModifier(modifier);
    else
        bee::Log::Warn("There is no {} in the {} template", magic_enum::enum_name(type), m_entityType);
}

void AttributesComponent::AddModifier(BaseAttributes type, ModifierType modType, double value,bool isBuff = false)
{
    if (m_attributes.find(type) != m_attributes.end()) m_attributes[type].AddModifier(StatModifier(modType, value,isBuff));
}

void AttributesComponent::SetTeam(const int teamId) { m_team = teamId; }

int AttributesComponent::GetTeam() const { return m_team; }
void AttributesComponent::SetEntityType(const std::string& entityType) { m_entityType = entityType; }
const std::string& AttributesComponent::GetEntityType() const { return m_entityType; }

void AttributesComponent::RemoveModifier(BaseAttributes type, const StatModifier& modifier)
{
    if (m_attributes.find(type) != m_attributes.end())
        m_attributes[type].RemoveModifier(modifier);
    else
        bee::Log::Warn("There is no {} in the {} template", magic_enum::enum_name(type), m_entityType);
}

void AttributesComponent::RemoveModifier(BaseAttributes type, ModifierType modType, double value, bool isBuff)
{
    for (auto& modifier : m_attributes[type].GetModifiers())
    {
        if (modifier.GetModifierType() != modType) continue;
        if (modifier.isBuff != isBuff) continue;
        if (modifier.GetValue() != value) continue;

        RemoveModifier(type, modifier);
        return;
    }
}

void AttributesComponent::ClearModifiers(BaseAttributes type)
{
    if (m_attributes.find(type) != m_attributes.end())
        m_attributes[type].ClearModifiers();
    else
        bee::Log::Warn("There is no {} in the {} template", magic_enum::enum_name(type), m_entityType);
}

void AttributesComponent::RemoveModifiersFromValue(BaseAttributes type, float amount,bool isBuff)
{
    const auto modifiers = GetModifiers(type);
    double remainingValue = amount;
    for (auto modifier : modifiers)
    {
        if (remainingValue <= 0) return;
        if (remainingValue >= modifier.GetValue())
        {
            remainingValue -= std::abs(modifier.GetValue());
            RemoveModifier(type,modifier.GetModifierType(),modifier.GetValue(),isBuff);
        }
        else
        {
            for (auto& mod : GetModifiers(type))
            {
                if (mod.GetValue() == modifier.GetValue() && mod.GetModifierType() == modifier.GetModifierType())
                {
                    mod.SetValue( modifier.GetValue() - remainingValue);
                }
            }
        }
    }
}
