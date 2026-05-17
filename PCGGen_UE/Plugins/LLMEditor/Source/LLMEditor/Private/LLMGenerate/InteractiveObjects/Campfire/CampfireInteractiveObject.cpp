#include "CampfireInteractiveObject.h"
#include "CustomModules.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

ACampfireInteractiveObject::ACampfireInteractiveObject()
{
}

void ACampfireInteractiveObject::BeginPlay()
{
 Super::BeginPlay();
}

void ACampfireInteractiveObject::HandleInteraction(AInteractiveCharacter* InteractingCharacter)
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
             TEXT("[CampfireInteractiveObject] %s was lit by %s."),
             *ObjName,
             *CharName));
 }

 // -----------------------------
 // 3. Subclass custom logic: Activate environment object and trigger quest event
 // -----------------------------
 auto* Environment = Manager.GetModule<FEnvironmentModule>();
 auto* Quest = Manager.GetModule<FQuestModule>();

 if (Environment)
 {
     Environment->Activate(ObjName);
 }

 if (Quest)
 {
     Quest->TriggerEvent(TEXT("campfire_lit"));
 }
}
