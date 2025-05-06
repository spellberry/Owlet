
#pragma once
#include <cereal/cereal.hpp>
#include <string>
#include <unordered_map>

enum class BaseAttributes
{
    Scale = 0,
    DiskScale = 1,
    WoodCost = 2,
    StoneCost = 3,
    CreationTime = 4,
    Damage = 5,
    AttackCooldown = 6,
    ProjectileSpeed = 7,
    ProjectileSize = 8,
    Range = 9,
    InterceptionRange = 10,
    MovementSpeed = 11,
    Armor = 12,
    HitPoints = 13,
    HPRegen = 14,
    DeathCooldown = 15,
    Height = 16,
    WoodBounty = 17,
    WoodBountyDeviation = 18,
    StoneBounty = 19,
    StoneBountyDeviation = 20,
    MaxTrainedUnitsQueued = 21,
    BuffRange=22,
    BuffValue=23,
    BuildingLevel=24,
    TimeReduction=25,
    CostReduction=26,
    SwordsmenLimitIncrease =27,
    MageLimitIncrease = 28,
    SelectionRange = 29
};

enum class ModifierType
{
    Additive,
    Multiplier
};

struct Corpse
{
    char dummy = 'D';
};

struct StatModifier
{
public:
    bool isBuff = false;
    StatModifier();
    StatModifier(ModifierType type, double value);
    StatModifier(ModifierType type, double value, bool buff);
    [[nodiscard]] double GetModifiedValue(double valueToModify) const;

    bool operator==(const StatModifier& rhs) const { return rhs.m_value == m_value && rhs.m_modifierType == m_modifierType; }
    bool operator!=(const StatModifier& rhs) const { return !operator==(rhs); }

    void SetValue(double value);
    [[nodiscard]] double GetValue() const;
    void SetModifierType(ModifierType type);
    [[nodiscard]] ModifierType GetModifierType() const;

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(m_value), CEREAL_NVP(m_modifierType));
    }

private:
    ModifierType m_modifierType = {};
    double m_value = 0;
};

class Attribute
{
public:
    Attribute(double value);
    Attribute();

    void SetBaseValue(const double toSet);
    double GetBaseValue() const;

    void AddModifier(const StatModifier modifier);
    void RemoveModifier(const StatModifier modifier);
    void ClearModifiers();
    void SetModifierValue(StatModifier modifier, double value);
    std::vector<StatModifier>& GetModifiers();
    std::vector<std::reference_wrapper<StatModifier>> GetModifiersOfType(ModifierType modType);

    double GetValue() const;

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(m_baseValue));
    }

private:
    double m_baseValue = 0.0;
    std::vector<StatModifier> m_modifiers;
};

struct AttributesComponent
{
    AttributesComponent() = default;
    const std::unordered_map<BaseAttributes, Attribute>& GetAttributes();
    void SetAttributes(const std::unordered_map<BaseAttributes, Attribute>& attributes);
    void SetAttributes(const std::unordered_map<BaseAttributes, double>& attributes)
    {
        for (auto& element : attributes)
        {
            SetAttribute(element.first, element.second);
        }
    };
    std::vector<StatModifier>& GetModifiers(BaseAttributes attributes) { return m_attributes[attributes].GetModifiers(); }

    void SetAttribute(BaseAttributes type, double value);
    void AddModifier(BaseAttributes type, StatModifier modifier);
    void AddModifier(BaseAttributes type, ModifierType modType, double value, bool isBuff);
    double GetValue(BaseAttributes type) const;
    void RemoveModifier(BaseAttributes type, const StatModifier& modifier);
    void RemoveModifier(BaseAttributes type, ModifierType modType, double value, bool isBuff);
    void ClearModifiers(BaseAttributes type);
    void RemoveModifiersFromValue(BaseAttributes type, float amount, bool isBuff);

    template <class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(m_entityType), CEREAL_NVP(m_team), CEREAL_NVP(smallGridIndex), CEREAL_NVP(flipped));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(m_entityType), CEREAL_NVP(m_team), CEREAL_NVP(smallGridIndex), CEREAL_NVP(flipped));
    }
    bool HasAttribute(BaseAttributes type) const { return m_attributes.find(type) != m_attributes.end(); }

    void SetTeam(int team);
    int GetTeam() const;
    void SetEntityType(const std::string& entityType);
    const std::string& GetEntityType() const;
    int smallGridIndex =
        -1;  // If the actor is a structure or prop, it should be snapped to a small grid point (on the "small/actor" grid).
    bool flipped =
        false;  // If the actor is a structure or prop, this flag is set to true if the actor is rotated at 90 or 270 degrees.
private:
    std::unordered_map<BaseAttributes, Attribute> m_attributes{};
    std::string m_entityType = "";
    int m_team = 2;
};