#pragma once
#include "CoreMinimal.h"
#include "IModuleBase.h"

/**
 * Environment module for managing interactive world objects
 * Handles state changes such as lighting campfires, opening chests, etc.
 */
class LLMEDITOR_API FEnvironmentModule : public IModuleBase
{
public:
    virtual void Initialize() override
    {
        UE_LOG(LogTemp, Log, TEXT("[EnvironmentModule] Initialized"));
    }

    virtual void Shutdown() override
    {
        UE_LOG(LogTemp, Log, TEXT("[EnvironmentModule] Shutdown"));
        TrackedObjects.Empty();
    }

    static FName StaticModuleName() { return TEXT("EnvironmentModule"); }

public:
    /** Activate an environment object by name (e.g. light campfire) */
    void Activate(const FString& ObjectName);

    /** Check if an object is currently active */
    bool IsObjectActive(const FString& ObjectName) const;

    /** Deactivate a specific environment object */
    void Deactivate(const FString& ObjectName);

private:
    /** Set of active environment objects */
    TSet<FString> TrackedObjects;
};
