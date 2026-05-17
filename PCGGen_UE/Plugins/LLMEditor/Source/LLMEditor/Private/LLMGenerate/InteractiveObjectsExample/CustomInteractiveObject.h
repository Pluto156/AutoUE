#pragma once
#include "InteractiveObjectBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "CustomInteractiveObject.generated.h"

UCLASS()
class ACustomInteractiveObject : public AInteractiveObjectBase
{
    GENERATED_BODY()
public:
    virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter) override;
public:
    ACustomInteractiveObject();
protected:
    /** °ó¶¨ËÀÍöÊÂ¼þ */
    virtual void BeginPlay() override;
};
