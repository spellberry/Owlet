#include "actors/units/unit_template.hpp"

#include "magic_enum/magic_enum_all.hpp"

UnitTemplate::UnitTemplate(UnitTemplatePresets preset) { InitializeAttributes(preset); }

void UnitTemplate::InitializeAttributes(UnitTemplatePresets preset)
{
    switch (preset)
    {
        case UnitTemplatePresets::None:
        {
            NoPreset();
            break;
        }
        case UnitTemplatePresets::AllyRangedUnit:
        {
            AllyRangedUnit();
            break;
        }
        case UnitTemplatePresets::AllyMeleeUnit:
        {
            AllyMeleeUnit();
            break;
        }
        case UnitTemplatePresets::EnemyRangedUnit:
        {
            EnemyRangedUnit();
            break;
        }
        case UnitTemplatePresets::EnemyMeleeUnit:
        {
            EnemyMeleeUnit();
            break;
        }
        default:
        {
            NoPreset();
            break;
        }
    }
}

void UnitTemplate::AddAttribute(BaseAttributes attribute, double value)
{
    m_baseAttributes.insert(std::pair<BaseAttributes, double>(attribute, value));
}

void UnitTemplate::NoPreset()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::MovementSpeed, 8.0);
    AddAttribute(BaseAttributes::HitPoints, 1.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);

    availableOrders.push_back(OrderType::Move);
}



void UnitTemplate::AllyRangedUnit()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::WoodCost, 0.0);
    AddAttribute(BaseAttributes::StoneCost, 0.0);
    AddAttribute(BaseAttributes::CreationTime, 1.0);
    AddAttribute(BaseAttributes::Damage, 1.0);
    AddAttribute(BaseAttributes::AttackCooldown, 1.0);
    AddAttribute(BaseAttributes::ProjectileSpeed, 25.0);
    AddAttribute(BaseAttributes::ProjectileSize, 1.0);
    AddAttribute(BaseAttributes::Range, 6.0);
    AddAttribute(BaseAttributes::InterceptionRange, 8.0);
    AddAttribute(BaseAttributes::MovementSpeed, 8.0);
    AddAttribute(BaseAttributes::Armor, 1.0);
    AddAttribute(BaseAttributes::HitPoints, 10.0);
    AddAttribute(BaseAttributes::HPRegen, 0.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);

    availableOrders.push_back(OrderType::Move);
    availableOrders.push_back(OrderType::Patrol);
    availableOrders.push_back(OrderType::OffensiveMove);
    availableOrders.push_back(OrderType::Attack);
}

void UnitTemplate::AllyMeleeUnit()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::WoodCost, 0.0);
    AddAttribute(BaseAttributes::StoneCost, 0.0);
    AddAttribute(BaseAttributes::CreationTime, 1.0);
    AddAttribute(BaseAttributes::Damage, 1.0);
    AddAttribute(BaseAttributes::AttackCooldown, 1.0);
    AddAttribute(BaseAttributes::InterceptionRange, 8.0);
    AddAttribute(BaseAttributes::MovementSpeed, 8.0);
    AddAttribute(BaseAttributes::Armor, 1.0);
    AddAttribute(BaseAttributes::HitPoints, 10.0);
    AddAttribute(BaseAttributes::HPRegen, 0.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);

    availableOrders.push_back(OrderType::Move);
    availableOrders.push_back(OrderType::Patrol);
    availableOrders.push_back(OrderType::OffensiveMove);
    availableOrders.push_back(OrderType::Attack);

    fsmPath = "default_fsm_melee.json";
}

void UnitTemplate::EnemyRangedUnit()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::WoodCost, 0.0);
    AddAttribute(BaseAttributes::StoneCost, 0.0);
    AddAttribute(BaseAttributes::CreationTime, 1.0);
    AddAttribute(BaseAttributes::Damage, 1.0);
    AddAttribute(BaseAttributes::AttackCooldown, 1.0);
    AddAttribute(BaseAttributes::ProjectileSpeed, 25.0);
    AddAttribute(BaseAttributes::ProjectileSize, 1.0);
    AddAttribute(BaseAttributes::Range, 6.0);
    AddAttribute(BaseAttributes::InterceptionRange, 8.0);
    AddAttribute(BaseAttributes::MovementSpeed, 8.0);
    AddAttribute(BaseAttributes::Armor, 1.0);
    AddAttribute(BaseAttributes::HitPoints, 10.0);
    AddAttribute(BaseAttributes::HPRegen, 0.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);
    AddAttribute(BaseAttributes::WoodBounty, 0.0);
    AddAttribute(BaseAttributes::StoneBounty, 0.0);

    availableOrders.push_back(OrderType::Move);
    availableOrders.push_back(OrderType::Patrol);
    availableOrders.push_back(OrderType::OffensiveMove);
    availableOrders.push_back(OrderType::Attack);
}

void UnitTemplate::EnemyMeleeUnit()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::WoodCost, 0.0);
    AddAttribute(BaseAttributes::StoneCost, 0.0);
    AddAttribute(BaseAttributes::CreationTime, 1.0);
    AddAttribute(BaseAttributes::Damage, 1.0);
    AddAttribute(BaseAttributes::AttackCooldown, 1.0);
    AddAttribute(BaseAttributes::InterceptionRange, 8.0);
    AddAttribute(BaseAttributes::MovementSpeed, 8.0);
    AddAttribute(BaseAttributes::Armor, 1.0);
    AddAttribute(BaseAttributes::HitPoints, 10.0);
    AddAttribute(BaseAttributes::HPRegen, 0.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);
    AddAttribute(BaseAttributes::WoodBounty, 0.0);
    AddAttribute(BaseAttributes::StoneBounty, 0.0);

    availableOrders.push_back(OrderType::Move);
    availableOrders.push_back(OrderType::Patrol);
    availableOrders.push_back(OrderType::OffensiveMove);
    availableOrders.push_back(OrderType::Attack);

    fsmPath = "default_fsm_melee.json";
}
