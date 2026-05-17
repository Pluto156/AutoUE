#pragma once
#include "InteractiveObjectBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "ChestInteractiveObject.generated.h"

UCLASS()
class LLMEDITOR_API AChestInteractiveObject : public AInteractiveObjectBase
{
 GENERATED_BODY()
public:
 virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter) override;
public:
 AChestInteractiveObject();
protected:
 /** Bind death events */
 virtual void BeginPlay() override;
};
