#pragma once
#include "InteractiveObjectBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "WoodenCabinInteractiveObject.generated.h"

UCLASS()
class LLMEDITOR_API AWoodenCabinInteractiveObject : public AInteractiveObjectBase
{
 GENERATED_BODY()
public:
 virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter) override;
public:
 AWoodenCabinInteractiveObject();
protected:
 /** Bind death events */
 virtual void BeginPlay() override;
};
