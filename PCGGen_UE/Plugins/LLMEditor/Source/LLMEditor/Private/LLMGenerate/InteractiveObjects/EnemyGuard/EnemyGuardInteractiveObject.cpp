#include "EnemyGuardInteractiveObject.h"
#include "CustomModules.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

AEnemyGuardInteractiveObject::AEnemyGuardInteractiveObject()
{
 bIsPartOfBattleSystem = true;
}

void AEnemyGuardInteractiveObject::BeginPlay()
{
 Super::BeginPlay();
}

void AEnemyGuardInteractiveObject::HandleInteraction(AInteractiveCharacter* InteractingCharacter)
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
             TEXT("[EnemyGuardInteractiveObject] %s entered combat with %s."),
             *ObjName,
             *CharName));
 }

 // -----------------------------
 // 3. Subclass custom logic: Combat engagement handled by base class via battle system
 // No additional module calls required here.
 // -----------------------------
}
