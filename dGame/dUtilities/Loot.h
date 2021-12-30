#pragma once

#include "dCommonVars.h"
#include <unordered_map>
#include "Singleton.h"
#include <vector>

#include "CDLootMatrixTable.h"
#include "CDLootTableTable.h"
#include "CDRarityTableTable.h"

class Entity;

using RarityTableEntry = CDRarityTable;
using LootMatrixEntry = CDLootMatrix;
using LootTableEntry = CDLootTable;
using RarityTable = std::vector<RarityTableEntry>;
using LootMatrix = std::vector<LootMatrixEntry>;
using LootTable = std::vector<LootTableEntry>;

// used for glue code with Entity and Player classes
namespace Loot {
    struct Info {
        LWOOBJID id;
        LOT lot;
        uint32_t count;
    };
}


class LootGenerator : public Singleton<LootGenerator> {
  public:
    LootGenerator() = default;

    std::unordered_map<LOT, int32_t> RollLootMatrix(Entity* player, uint32_t matrixIndex);
    std::unordered_map<LOT, int32_t> RollLootMatrix(uint32_t matrixIndex);
    void GiveLoot(Entity* player, uint32_t matrixIndex);
    void GiveLoot(Entity* player, std::unordered_map<LOT, int32_t>& result);
    void GiveActivityLoot(Entity* player, Entity* source, uint32_t activityID, int32_t rating = 0);
    void DropLoot(Entity* player, Entity* killedObject, uint32_t matrixIndex, uint32_t minCoins, uint32_t maxCoins);
    void DropLoot(Entity* player, Entity* killedObject, std::unordered_map<LOT, int32_t>& result, uint32_t minCoins, uint32_t maxCoins);
    void DropActivityLoot(Entity* player, Entity* source, uint32_t activityID, int32_t rating = 0);

  private:
    uint32_t LookupItemRarity(uint32_t itemID);
    RarityTable LookupRarityTable(uint32_t index);
    LootMatrix LookupLootMatrix(uint32_t index);
    LootTable LookupLootTable(uint32_t index);
};
