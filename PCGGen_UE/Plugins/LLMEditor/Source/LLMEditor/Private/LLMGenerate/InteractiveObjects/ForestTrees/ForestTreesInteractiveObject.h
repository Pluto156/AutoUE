#pragma once
#include "InteractiveObjectBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "ForestTreesInteractiveObject.generated.h"

UCLASS()
class LLMEDITOR_API AForestTreesInteractiveObject : public AInteractiveObjectBase
{
 GENERATED_BODY()
public:
 virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter) override;
public:
 AForestTreesInteractiveObject();
protected:
 /** Bind death events */
 virtual void BeginPlay() override;
};
