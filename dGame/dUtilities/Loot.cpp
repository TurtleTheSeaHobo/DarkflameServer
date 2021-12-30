#include <algorithm>

#include "Loot.h"

#include "CDComponentsRegistryTable.h"
#include "CDItemComponentTable.h"
#include "Character.h"
#include "Entity.h"
#include "GameMessages.h"
#include "GeneralUtils.h"
#include "InventoryComponent.h"
#include "MissionComponent.h"
#include "dLogger.h"

std::unordered_map<LOT, int32_t> LootGenerator::RollLootMatrix(Entity* player, uint32_t matrixIndex) {
    auto* missionComponent = player->GetComponent<MissionComponent>();

    std::unordered_map<LOT, int32_t> drops;

    if (missionComponent == nullptr) {
        return drops;
    }

    Game::logger->Log("LootGenerator", "Rolling loot matrix %u for player %i...", matrixIndex, player->GetLOT());

    //const LootMatrix& matrix = m_LootMatrices[matrixIndex];
    LootMatrix matrix = LookupLootMatrix(matrixIndex);

    Game::logger->Log("LootGenerator", "LootMatrix lookup okay!");

    for (const LootMatrixEntry& entry : matrix) {
        if (GeneralUtils::GenerateRandomNumber<float>(0, 1) < entry.percent) {
            Game::logger->Log("LootGenerator", "Rolled to drop LootMatrixEntry with id %u", entry.id);

            LootTable lootTable = LookupLootTable(entry.LootTableIndex);

            Game::logger->Log("LootGenerator", "LootTable lookup okay!");

            RarityTable rarityTable = LookupRarityTable(entry.RarityTableIndex);

            Game::logger->Log("LootGenerator", "RarityTable lookup okay!");

            uint32_t dropCount = GeneralUtils::GenerateRandomNumber<uint32_t>(entry.minToDrop, entry.maxToDrop);
            for (uint32_t i = 0; i < dropCount; ++i) {
                uint32_t maxRarity = 1;

                float rarityRoll = GeneralUtils::GenerateRandomNumber<float>(0, 1);

                for (const RarityTableEntry& rarity : rarityTable) {
                    if (rarity.randmax >= rarityRoll) {
                        maxRarity = rarity.rarity;
                    } else {
                        break;
                    }
                }

                bool rarityFound = false;
                std::vector<LootTableEntry> possibleDrops;

                for (const LootTableEntry& loot : lootTable) {
                    uint32_t rarity = LookupItemRarity(loot.itemid);

                    Game::logger->Log("LootGenerator", "Item rarity lookup for item id %u okay!", loot.itemid);

                    if (rarity == maxRarity) {
                        possibleDrops.push_back(loot);
                        rarityFound = true;
                    } else if (rarity < maxRarity && !rarityFound) {
                        possibleDrops.push_back(loot);
                        maxRarity = rarity;
                    }
                }

                if (possibleDrops.size() > 0) {
                    LootTableEntry drop = possibleDrops[GeneralUtils::GenerateRandomNumber<uint32_t>(0, possibleDrops.size() - 1)];

                    // filter out uneeded mission items
                    if (drop.MissionDrop && !missionComponent->RequiresItem(drop.itemid))
                        continue;

                    // convert faction token proxy
                    if (drop.itemid == 13763) {
                        if (missionComponent->GetMissionState(545) == MissionState::MISSION_STATE_COMPLETE)
                            drop.itemid = 8318; // "Assembly Token"
                        else if (missionComponent->GetMissionState(556) == MissionState::MISSION_STATE_COMPLETE)
                            drop.itemid = 8321; // "Venture League Token"
                        else if (missionComponent->GetMissionState(567) == MissionState::MISSION_STATE_COMPLETE)
                            drop.itemid = 8319; // "Sentinels Token"
                        else if (missionComponent->GetMissionState(578) == MissionState::MISSION_STATE_COMPLETE)
                            drop.itemid = 8320; // "Paradox Token"
                    }

                    if (drop.itemid == 13763) {
                        continue;
                    } // check if we aren't in faction

                    if (drops.find(drop.itemid) == drops.end()) {
                        drops.insert({drop.itemid, 1});
                    } else {
                        ++drops[drop.itemid];
                    }
                }
            }
        }
    }

    return drops;
}

std::unordered_map<LOT, int32_t> LootGenerator::RollLootMatrix(uint32_t matrixIndex) {
    std::unordered_map<LOT, int32_t> drops;

    Game::logger->Log("LootGenerator", "Rolling loot matrix %u...", matrixIndex);

    LootMatrix matrix = LookupLootMatrix(matrixIndex);

    Game::logger->Log("LootGenerator", "LootMatrix lookup okay!");

    for (const LootMatrixEntry& entry : matrix) {
        if (GeneralUtils::GenerateRandomNumber<float>(0, 1) < entry.percent) {
            Game::logger->Log("LootGenerator", "Rolled to drop LootMatrixEntry with id %u", entry.id);

            LootTable lootTable = LookupLootTable(entry.LootTableIndex);

            Game::logger->Log("LootGenerator", "LootTable lookup okay!");

            RarityTable rarityTable = LookupRarityTable(entry.RarityTableIndex);

            Game::logger->Log("LootGenerator", "RarityTable lookup okay!");

            uint32_t dropCount = GeneralUtils::GenerateRandomNumber<uint32_t>(entry.minToDrop, entry.maxToDrop);
            for (uint32_t i = 0; i < dropCount; ++i) {
                uint32_t maxRarity = 1;

                float rarityRoll = GeneralUtils::GenerateRandomNumber<float>(0, 1);

                for (const RarityTableEntry& rarity : rarityTable) {
                    if (rarity.randmax >= rarityRoll) {
                        maxRarity = rarity.rarity;
                    } else {
                        break;
                    }
                }

                bool rarityFound = false;
                std::vector<LootTableEntry> possibleDrops;

                for (const LootTableEntry& loot : lootTable) {
                    uint32_t rarity = LookupItemRarity(loot.itemid);

                    Game::logger->Log("LootGenerator", "Item rarity lookup for item id %u okay!", loot.itemid);

                    if (rarity == maxRarity) {
                        possibleDrops.push_back(loot);
                        rarityFound = true;
                    } else if (rarity < maxRarity && !rarityFound) {
                        possibleDrops.push_back(loot);
                        maxRarity = rarity;
                    }
                }

                if (possibleDrops.size() > 0) {
                    LootTableEntry drop = possibleDrops[GeneralUtils::GenerateRandomNumber<uint32_t>(0, possibleDrops.size() - 1)];

                    if (drops.find(drop.itemid) == drops.end()) {
                        drops.insert({drop.itemid, 1});
                    } else {
                        ++drops[drop.itemid];
                    }
                }
            }
        }
    }

    return drops;
}

void LootGenerator::GiveLoot(Entity* player, uint32_t matrixIndex) {
    player = player->GetOwner(); // If the owner is overwritten, we collect that here

    std::unordered_map<LOT, int32_t> result = RollLootMatrix(player, matrixIndex);

    GiveLoot(player, result);
}

void LootGenerator::GiveLoot(Entity* player, std::unordered_map<LOT, int32_t>& result) {
    player = player->GetOwner(); // if the owner is overwritten, we collect that here

    auto* inventoryComponent = player->GetComponent<InventoryComponent>();

    if (!inventoryComponent)
        return;

    for (const auto& pair : result) {
        inventoryComponent->AddItem(pair.first, pair.second);
    }
}

void LootGenerator::GiveActivityLoot(Entity* player, Entity* source, uint32_t activityID, int32_t rating) {
    CDActivityRewardsTable* activityRewardsTable = CDClientManager::Instance()->GetTable<CDActivityRewardsTable>("ActivityRewards");
    std::vector<CDActivityRewards> activityRewards = activityRewardsTable->Query([activityID](CDActivityRewards entry) { return (entry.objectTemplate == activityID); });

    const CDActivityRewards* selectedReward = nullptr;
    for (const auto& activityReward : activityRewards) {
        if (activityReward.activityRating <= rating && (selectedReward == nullptr || activityReward.activityRating > selectedReward->activityRating)) {
            selectedReward = &activityReward;
        }
    }

    if (!selectedReward)
        return;

    uint32_t minCoins = 0;
    uint32_t maxCoins = 0;

    CDCurrencyTableTable* currencyTableTable = CDClientManager::Instance()->GetTable<CDCurrencyTableTable>("CurrencyTable");
    std::vector<CDCurrencyTable> currencyTable = currencyTableTable->Query([selectedReward](CDCurrencyTable entry) { return (entry.currencyIndex == selectedReward->CurrencyIndex && entry.npcminlevel == 1); });

    if (currencyTable.size() > 0) {
        minCoins = currencyTable[0].minvalue;
        maxCoins = currencyTable[0].maxvalue;
    }

    GiveLoot(player, selectedReward->LootMatrixIndex);

    uint32_t coins = (int)(minCoins + GeneralUtils::GenerateRandomNumber<float>(0, 1) * (maxCoins - minCoins));

    auto* character = player->GetCharacter();

    character->SetCoins(character->GetCoins() + coins);
}

void LootGenerator::DropLoot(Entity* player, Entity* killedObject, uint32_t matrixIndex, uint32_t minCoins, uint32_t maxCoins) {
    player = player->GetOwner(); // if the owner is overwritten, we collect that here

    auto* inventoryComponent = player->GetComponent<InventoryComponent>();

    if (!inventoryComponent)
        return;

    std::unordered_map<LOT, int32_t> result = RollLootMatrix(player, matrixIndex);

    DropLoot(player, killedObject, result, minCoins, maxCoins);
}

void LootGenerator::DropLoot(Entity* player, Entity* killedObject, std::unordered_map<LOT, int32_t>& result, uint32_t minCoins, uint32_t maxCoins) {
    player = player->GetOwner(); // if the owner is overwritten, we collect that here

    auto* inventoryComponent = player->GetComponent<InventoryComponent>();

    if (!inventoryComponent)
        return;

    const auto spawnPosition = killedObject->GetPosition();

    const auto source = killedObject->GetObjectID();

    for (const auto& pair : result) {
        for (int i = 0; i < pair.second; ++i) {
            GameMessages::SendDropClientLoot(player, source, pair.first, 0, spawnPosition, 1);
        }
    }

    uint32_t coins = (int)(minCoins + GeneralUtils::GenerateRandomNumber<float>(0, 1) * (maxCoins - minCoins));

    GameMessages::SendDropClientLoot(player, source, LOT_NULL, coins, spawnPosition);
}

void LootGenerator::DropActivityLoot(Entity* player, Entity* source, uint32_t activityID, int32_t rating) {
    CDActivityRewardsTable* activityRewardsTable = CDClientManager::Instance()->GetTable<CDActivityRewardsTable>("ActivityRewards");
    std::vector<CDActivityRewards> activityRewards = activityRewardsTable->Query([activityID](CDActivityRewards entry) { return (entry.objectTemplate == activityID); });

    const CDActivityRewards* selectedReward = nullptr;
    for (const auto& activityReward : activityRewards) {
        if (activityReward.activityRating <= rating && (selectedReward == nullptr || activityReward.activityRating > selectedReward->activityRating)) {
            selectedReward = &activityReward;
        }
    }

    if (selectedReward == nullptr) {
        return;
    }

    uint32_t minCoins = 0;
    uint32_t maxCoins = 0;

    CDCurrencyTableTable* currencyTableTable = CDClientManager::Instance()->GetTable<CDCurrencyTableTable>("CurrencyTable");
    std::vector<CDCurrencyTable> currencyTable = currencyTableTable->Query([selectedReward](CDCurrencyTable entry) { return (entry.currencyIndex == selectedReward->CurrencyIndex && entry.npcminlevel == 1); });

    if (currencyTable.size() > 0) {
        minCoins = currencyTable[0].minvalue;
        maxCoins = currencyTable[0].maxvalue;
    }

    DropLoot(player, source, selectedReward->LootMatrixIndex, minCoins, maxCoins);
}

uint32_t LootGenerator::LookupItemRarity(uint32_t itemID) {
    auto* componentsRegistryTable = CDClientManager::Instance()->GetTable<CDComponentsRegistryTable>("ComponentsRegistry");
    auto* itemComponentTable = CDClientManager::Instance()->GetTable<CDItemComponentTable>("ItemComponent");

    uint32_t itemComponentID = componentsRegistryTable->GetByIDAndType(itemID, COMPONENT_TYPE_ITEM);
    const CDItemComponent& item = itemComponentTable->GetItemComponentByID(itemComponentID);

    return item.rarity;
}

RarityTable LootGenerator::LookupRarityTable(uint32_t index) {
    auto* rarityTableTable = CDClientManager::Instance()->GetTable<CDRarityTableTable>("RarityTable");
    std::vector<CDRarityTable> rarityTable = rarityTableTable->Query([index](const CDRarityTable& entry) { return entry.RarityTableIndex == index; });

    return rarityTable;
}

LootMatrix LootGenerator::LookupLootMatrix(uint32_t index) {
    auto* lootMatrixTable = CDClientManager::Instance()->GetTable<CDLootMatrixTable>("LootMatrixTable");
    std::vector<CDLootMatrix> lootMatrix = lootMatrixTable->Query([index](const CDLootMatrix& entry) { return entry.LootMatrixIndex == index; });

    return lootMatrix;
}

LootTable LootGenerator::LookupLootTable(uint32_t index) {
    auto* lootTableTable = CDClientManager::Instance()->GetTable<CDLootTableTable>("LootTable");
    std::vector<CDLootTable> lootTable = lootTableTable->Query([index](const CDLootTable& entry) { return entry.LootTableIndex == index; });

    return lootTable;
}