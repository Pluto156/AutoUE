#include "GroundVegetationInteractiveObject.h"
#include "CustomModules.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

AGroundVegetationInteractiveObject::AGroundVegetationInteractiveObject()
{
}

void AGroundVegetationInteractiveObject::BeginPlay()
{
 Super::BeginPlay();
}

void AGroundVegetationInteractiveObject::HandleInteraction(AInteractiveCharacter* InteractingCharacter)
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
             TEXT("[GroundVegetationInteractiveObject] %s was interacted with by %s, but is non-interactive."),
             *ObjName,
             *CharName));
 }
}
