#include "RocksInteractiveObject.h"
#include "CustomModules.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

ARocksInteractiveObject::ARocksInteractiveObject()
{
}

void ARocksInteractiveObject::BeginPlay()
{
 Super::BeginPlay();
}

void ARocksInteractiveObject::HandleInteraction(AInteractiveCharacter* InteractingCharacter)
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
             TEXT("[RocksInteractiveObject] %s was interacted with by %s, but is non-interactive."),
             *ObjName,
             *CharName));
 }
}
