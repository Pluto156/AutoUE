#pragma once
#include "InteractiveObjectBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "CampfireInteractiveObject.generated.h"

UCLASS()
class LLMEDITOR_API ACampfireInteractiveObject : public AInteractiveObjectBase
{
 GENERATED_BODY()
public:
 virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter) override;
public:
 ACampfireInteractiveObject();
protected:
 /** Bind death events */
 virtual void BeginPlay() override;
};
