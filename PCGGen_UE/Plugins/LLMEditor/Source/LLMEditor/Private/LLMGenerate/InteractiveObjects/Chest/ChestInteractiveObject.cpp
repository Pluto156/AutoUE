#include "ChestInteractiveObject.h"
#include "CustomModules.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

AChestInteractiveObject::AChestInteractiveObject()
{
}

void AChestInteractiveObject::BeginPlay()
{
 Super::BeginPlay();
}

void AChestInteractiveObject::HandleInteraction(AInteractiveCharacter* InteractingCharacter)
{
 // -----------------------------
 // 1. Execute base interaction logic (includes basic logging)
 // -----------------------------
 Super::HandleInteraction(InteractingCharacter);

 // -----------------------------
 // 2. Record subclass-specific interaction logic
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
             TEXT("[ChestInteractiveObject] %s opened by %s."),
             *ObjName,
             *CharName));
 }

 // -----------------------------
 // 3. Subclass custom logic: Add item to inventory and activate environment object
 // -----------------------------
 auto* Inventory = Manager.GetModule<FInventoryModule>();
 auto* Environment = Manager.GetModule<FEnvironmentModule>();

 if (Inventory)
 {
     Inventory->AddItem(ObjName);
 }

 if (Environment)
 {
     Environment->Activate(ObjName);
 }
}
