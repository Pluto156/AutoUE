#pragma once

#include "UObject/Interface.h"
#include "BattleInterface.generated.h"

UINTERFACE(BlueprintType)
class UBattleInterface : public UInterface
{
    GENERATED_BODY()
};

class IBattleInterface
{
    GENERATED_BODY()

public:
    /** 所有可参与战斗的对象都必须提供自己的状态组件 */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Battle")
    class UBattleStatComponent* GetBattleStatComponent() const;
};
