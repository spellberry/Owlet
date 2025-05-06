#include "actors/structures/structure_template.hpp"

#include "magic_enum/magic_enum_all.hpp"

StructureTemplate::StructureTemplate(StructureTemplatePresets preset)
{
    InitializeAttributes(preset);
}

void StructureTemplate::InitializeAttributes(StructureTemplatePresets preset)
{
    switch (preset)
    {
        case StructureTemplatePresets::None:
        {
            NoPreset();
            break;
        }
        case StructureTemplatePresets::ProductionStructure:
        {
            ProductionStructure();
            break;
        }
        case StructureTemplatePresets::AttackTower:
        {
            AttackTower();
            break;
        }
        case StructureTemplatePresets::Wall:
        {
            Wall();
            break;
        }
        case StructureTemplatePresets::BuffStructure:
        {
            BuffTower();
            break;
        }
        default:
        {
            NoPreset();
            break;
        }
    }
}

void StructureTemplate::AddAttribute(BaseAttributes attribute, double value)
{
    m_baseAttributes.insert(std::pair<BaseAttributes, double>(attribute, value));
}

void StructureTemplate::NoPreset()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::HitPoints, 10.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);

    fsmPath = "barracks_fsm.json";
}

void StructureTemplate::ProductionStructure()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::WoodCost, 0.0);
    AddAttribute(BaseAttributes::StoneCost, 0.0);
    AddAttribute(BaseAttributes::CreationTime, 1.0);
    AddAttribute(BaseAttributes::Armor, 1.0);
    AddAttribute(BaseAttributes::HitPoints, 25.0);
    AddAttribute(BaseAttributes::HPRegen, 0.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);
    AddAttribute(BaseAttributes::Height, 2.0);

    availableOrders.push_back(OrderType::TrainWarrior);

    fsmPath = "barracks_fsm.json";
}

void StructureTemplate::AttackTower()
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
    AddAttribute(BaseAttributes::Armor, 1.0);
    AddAttribute(BaseAttributes::HitPoints, 25.0);
    AddAttribute(BaseAttributes::HPRegen, 0.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);
    AddAttribute(BaseAttributes::Height, 2.0);

    availableOrders.push_back(OrderType::Attack);

    fsmPath = "bunker_fsm.json";
}

void StructureTemplate::Wall()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::WoodCost, 0.0);
    AddAttribute(BaseAttributes::StoneCost, 0.0);
    AddAttribute(BaseAttributes::CreationTime, 1.0);
    AddAttribute(BaseAttributes::Armor, 3.0);
    AddAttribute(BaseAttributes::HitPoints, 25.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);

    fsmPath = "barracks_fsm.json";
}

void StructureTemplate::BuffTower()
{
    AddAttribute(BaseAttributes::Scale, 1.0);
    AddAttribute(BaseAttributes::DiskScale, 1.0);
    AddAttribute(BaseAttributes::WoodCost, 0.0);
    AddAttribute(BaseAttributes::StoneCost, 0.0);
    AddAttribute(BaseAttributes::CreationTime, 1.0);
    AddAttribute(BaseAttributes::Range, 6.0);
    AddAttribute(BaseAttributes::Armor, 1.0);
    AddAttribute(BaseAttributes::HitPoints, 25.0);
    AddAttribute(BaseAttributes::DeathCooldown, 0.5);
    AddAttribute(BaseAttributes::Height, 2.0);
    AddAttribute(BaseAttributes::BuffRange, 5.0f);
    AddAttribute(BaseAttributes::BuffValue, 10.0f);

    availableOrders.push_back(OrderType::None);

    fsmPath = "dummy_fsm.json";
}
