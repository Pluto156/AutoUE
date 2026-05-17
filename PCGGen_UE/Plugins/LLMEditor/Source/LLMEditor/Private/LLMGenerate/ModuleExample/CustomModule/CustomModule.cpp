#include "CustomModule.h"
#include "ModuleRegistry.h"
#include "ModuleMacro.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

void FCustomModule::AddItem(const FString& ItemName)
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (ItemName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CustomModule] Invalid item name."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[CustomModule] AddItem failed: invalid item name."));
        }
        return;
    }

    int32& Count = Items.FindOrAdd(ItemName);
    Count++;

    UE_LOG(LogTemp, Log,
        TEXT("[CustomModule] Added item: %s (x%d)"),
        *ItemName, Count);

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[CustomModule] Item added: %s, new count = %d."),
                *ItemName,
                Count));
    }

    PrintInventory();
}

void FCustomModule::PrintInventory() const
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (Items.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[CustomModule] Inventory is empty."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[CustomModule] Inventory is empty."));
        }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("=== Current Inventory ==="));

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(TEXT("[CustomModule] Inventory snapshot begin."));
    }

    for (const auto& Pair : Items)
    {
        UE_LOG(LogTemp, Log,
            TEXT(" - %s : %d"),
            *Pair.Key,
            Pair.Value);

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                FString::Printf(
                    TEXT("[CustomModule] Inventory entry: %s = %d."),
                    *Pair.Key,
                    Pair.Value));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("=========================="));

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(TEXT("[CustomModule] Inventory snapshot end."));
    }
}
/** ×¢²áÄ£¿é */
REGISTER_MODULE(FCustomModule)
