#pragma once
#include "CoreMinimal.h"
#include "IModuleBase.h"

/**
 * Quest module for managing and triggering quest events
 * Responds to environmental interactions (e.g. lighting campfire)
 * Advances active quests by triggering named events such as 'campfire_lit'
 */
class LLMEDITOR_API FQuestModule : public IModuleBase
{
public:
    virtual void Initialize() override
    {
        UE_LOG(LogTemp, Log, TEXT("[QuestModule] Initialized"));
    }

    virtual void Shutdown() override
    {
        UE_LOG(LogTemp, Log, TEXT("[QuestModule] Shutdown"));
        ActiveEvents.Empty();
    }

    static FName StaticModuleName() { return TEXT("QuestModule"); }

public:
    /** Trigger a quest event by name (e.g. 'campfire_lit') */
    void TriggerEvent(const FString& EventName);

    /** Check if a specific event has been triggered */
    bool HasEventOccurred(const FString& EventName) const;

private:
    /** Set of events that have been triggered */
    TSet<FString> ActiveEvents;
};
