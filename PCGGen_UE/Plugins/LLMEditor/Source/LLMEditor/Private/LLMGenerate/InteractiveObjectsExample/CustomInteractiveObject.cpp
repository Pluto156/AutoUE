#include "CustomInteractiveObject.h"
#include "CustomModules.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"
ACustomInteractiveObject::ACustomInteractiveObject()
{
}
void ACustomInteractiveObject::BeginPlay()
{
    Super::BeginPlay();
}

void ACustomInteractiveObject::HandleInteraction(AInteractiveCharacter* InteractingCharacter)
{
    // -----------------------------
    // 1. 先执行父类交互逻辑（含基础日志）
    // -----------------------------
    Super::HandleInteraction(InteractingCharacter);

    // -----------------------------
    // 2. 记录子类自定义交互逻辑
    // -----------------------------
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    const FString ObjName = GetName();
    const FString CharName = InteractingCharacter
        ? InteractingCharacter->GetName()
        : TEXT("Unknown");

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[CustomInteractiveObject] %s executed custom interaction logic with %s."),
                *ObjName,
                *CharName));
    }

    // -----------------------------
    // 3. 子类自己的逻辑（示例）
    // -----------------------------
    // auto* Inventory = Manager.GetModule<FInventoryModule>();
    // if (Inventory)
    // {
    //     Inventory->AddItem(GetName());
    // }
}
