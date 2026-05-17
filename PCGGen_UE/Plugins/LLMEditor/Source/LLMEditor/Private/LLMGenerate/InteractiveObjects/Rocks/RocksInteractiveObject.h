#pragma once
#include "InteractiveObjectBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "RocksInteractiveObject.generated.h"

UCLASS()
class LLMEDITOR_API ARocksInteractiveObject : public AInteractiveObjectBase
{
 GENERATED_BODY()
public:
 virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter) override;
public:
 ARocksInteractiveObject();
protected:
 /** Bind death events */
 virtual void BeginPlay() override;
};
