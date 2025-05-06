#pragma once
enum class OrderType
{
    None=0,
    Move=1,
    OffensiveMove=2,
    Patrol=3,
    Attack=4,

    BuildBase=5,
    BuildMageTower=6,
    BuildBarracks=7,
    BuildWallStart=8,
    BuildWallEnd=9,

    TrainWarrior=10,
    TrainMage=11,
    UpgradeBuilding=12,

    BuildFenceStart = 13,
    BuildFenceEnd = 14
};
