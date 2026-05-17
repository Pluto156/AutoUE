#pragma once
#include "InteractiveObjectBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "GroundVegetationInteractiveObject.generated.h"

UCLASS()
class LLMEDITOR_API AGroundVegetationInteractiveObject : public AInteractiveObjectBase
{
 GENERATED_BODY()
public:
 virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter) override;
public:
 AGroundVegetationInteractiveObject();
protected:
 /** Bind death events */
 virtual void BeginPlay() override;
};
