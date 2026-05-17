#pragma once
#include "CoreMinimal.h"
#include "IModuleBase.h"

/**
 * Inventory module for managing player items
 * Handles item acquisition and storage from interactions
 */
class LLMEDITOR_API FInventoryModule : public IModuleBase
{
public:
    virtual void Initialize() override
    {
        UE_LOG(LogTemp, Log, TEXT("[InventoryModule] Initialized"));
    }

    virtual void Shutdown() override
    {
        UE_LOG(LogTemp, Log, TEXT("[InventoryModule] Shutdown"));
        PlayerInventory.Empty();
    }

    static FName StaticModuleName() { return TEXT("InventoryModule"); }

public:
    /** Add an item to the player's inventory */
    void AddItem(const FString& ItemName);

    /** Query the current count of a specific item */
    int32 GetItemCount(const FString& ItemName) const;

    /** Display all items in inventory */
    void ListInventory() const;

private:
    /** Player's current inventory */
    TMap<FString, int32> PlayerInventory;
};
