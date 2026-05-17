#include "WoodenCabinInteractiveObject.h"
#include "CustomModules.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

AWoodenCabinInteractiveObject::AWoodenCabinInteractiveObject()
{
}

void AWoodenCabinInteractiveObject::BeginPlay()
{
 Super::BeginPlay();
}

void AWoodenCabinInteractiveObject::HandleInteraction(AInteractiveCharacter* InteractingCharacter)
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
             TEXT("[WoodenCabinInteractiveObject] %s was interacted with by %s, but no further action taken."),
             *ObjName,
             *CharName));
 }
}
