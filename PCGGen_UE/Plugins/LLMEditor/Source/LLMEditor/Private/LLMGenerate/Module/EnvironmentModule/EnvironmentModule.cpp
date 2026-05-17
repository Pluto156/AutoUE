#include "EnvironmentModule.h"
#include "ModuleRegistry.h"
#include "ModuleMacro.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"
#include "CustomModules.h"

void FEnvironmentModule::Activate(const FString& ObjectName)
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (ObjectName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EnvironmentModule] Cannot activate invalid object name."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[EnvironmentModule] Activate failed: invalid object name."));
        }
        return;
    }

    if (TrackedObjects.Contains(ObjectName))
    {
        UE_LOG(LogTemp, Log,
            TEXT("[EnvironmentModule] Object already active: %s"),
            *ObjectName);

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                FString::Printf(
                    TEXT("[EnvironmentModule] Attempted to activate already active object: %s."),
                    *ObjectName));
        }
        return;
    }

    TrackedObjects.Add(ObjectName);

    UE_LOG(LogTemp, Log,
        TEXT("[EnvironmentModule] Activated environment object: %s"),
        *ObjectName);

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[EnvironmentModule] Object activated: %s."),
                *ObjectName));
    }
}

bool FEnvironmentModule::IsObjectActive(const FString& ObjectName) const
{
    const bool bIsActive = TrackedObjects.Contains(ObjectName);

    UE_LOG(LogTemp, Log,
        TEXT("[EnvironmentModule] Object '%s' active status: %s"),
        *ObjectName,
        bIsActive ? TEXT("true") : TEXT("false"));

    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[EnvironmentModule] Query object status: %s = %s."),
                *ObjectName,
                bIsActive ? TEXT("active") : TEXT("inactive")));
    }

    return bIsActive;
}

void FEnvironmentModule::Deactivate(const FString& ObjectName)
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (ObjectName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EnvironmentModule] Cannot deactivate invalid object name."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[EnvironmentModule] Deactivate failed: invalid object name."));
        }
        return;
    }

    if (!TrackedObjects.Contains(ObjectName))
    {
        UE_LOG(LogTemp, Log,
            TEXT("[EnvironmentModule] Object not active, cannot deactivate: %s"),
            *ObjectName);

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                FString::Printf(
                    TEXT("[EnvironmentModule] Attempted to deactivate inactive object: %s."),
                    *ObjectName));
        }
        return;
    }

    TrackedObjects.Remove(ObjectName);

    UE_LOG(LogTemp, Log,
        TEXT("[EnvironmentModule] Deactivated environment object: %s"),
        *ObjectName);

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[EnvironmentModule] Object deactivated: %s."),
                *ObjectName));
    }
}

/** Register module */
REGISTER_MODULE(FEnvironmentModule)
