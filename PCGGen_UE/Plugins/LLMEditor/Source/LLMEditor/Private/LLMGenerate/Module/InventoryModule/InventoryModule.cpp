#include "InventoryModule.h"
#include "ModuleRegistry.h"
#include "ModuleMacro.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"
#include "CustomModules.h"

void FInventoryModule::AddItem(const FString& ItemName)
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (ItemName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[InventoryModule] Cannot add empty item name."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[InventoryModule] AddItem failed: invalid item name."));
        }
        return;
    }

    int32& Count = PlayerInventory.FindOrAdd(ItemName);
    Count++;

    UE_LOG(LogTemp, Log,
        TEXT("[InventoryModule] Added item: %s (x%d)"),
        *ItemName, Count);

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[InventoryModule] Item added: %s, new count = %d."),
                *ItemName,
                Count));
    }

    ListInventory();
}

int32 FInventoryModule::GetItemCount(const FString& ItemName) const
{
    if (PlayerInventory.Contains(ItemName))
    {
        return PlayerInventory.FindRef(ItemName);
    }
    return 0;
}

void FInventoryModule::ListInventory() const
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (PlayerInventory.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[InventoryModule] Inventory is empty."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[InventoryModule] Inventory is empty."));
        }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("=== Player Inventory ==="));

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(TEXT("[InventoryModule] Listing inventory contents:"));
    }

    for (const auto& Pair : PlayerInventory)
    {
        UE_LOG(LogTemp, Log,
            TEXT(" - %s : %d"),
            *Pair.Key,
            Pair.Value);

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                FString::Printf(
                    TEXT("[InventoryModule] Item: %s, Count: %d"),
                    *Pair.Key,
                    Pair.Value));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("========================"));
}

/** Register module */
REGISTER_MODULE(FInventoryModule)
