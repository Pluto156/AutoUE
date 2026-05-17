#include "QuestModule.h"
#include "ModuleRegistry.h"
#include "ModuleMacro.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"
#include "CustomModules.h"

void FQuestModule::TriggerEvent(const FString& EventName)
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (EventName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[QuestModule] Cannot trigger empty event name."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[QuestModule] TriggerEvent failed: invalid event name."));
        }
        return;
    }

    if (ActiveEvents.Contains(EventName))
    {
        UE_LOG(LogTemp, Log,
            TEXT("[QuestModule] Event already triggered: %s"),
            *EventName);

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                FString::Printf(
                    TEXT("[QuestModule] Attempted to re-trigger event: %s."),
                    *EventName));
        }
        return;
    }

    ActiveEvents.Add(EventName);

    UE_LOG(LogTemp, Log,
        TEXT("[QuestModule] Quest event triggered: %s"),
        *EventName);

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[QuestModule] Event triggered: %s."),
                *EventName));
    }
}

bool FQuestModule::HasEventOccurred(const FString& EventName) const
{
    const bool bOccurred = ActiveEvents.Contains(EventName);

    UE_LOG(LogTemp, Log,
        TEXT("[QuestModule] Event '%s' occurred status: %s"),
        *EventName,
        bOccurred ? TEXT("true") : TEXT("false"));

    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[QuestModule] Queried event status: %s = %s."),
                *EventName,
                bOccurred ? TEXT("occurred") : TEXT("not occurred")));
    }

    return bOccurred;
}

/** Register module */
REGISTER_MODULE(FQuestModule)
