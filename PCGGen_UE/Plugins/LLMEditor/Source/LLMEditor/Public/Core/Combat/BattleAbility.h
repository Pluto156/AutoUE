#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BattleAbility.generated.h"

USTRUCT(BlueprintType)
struct FBattleAbilityContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Context")
    AActor* InstigatorActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Context")
    AActor* TargetActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Context")
    FVector Direction = FVector::ZeroVector;
};


UCLASS(Blueprintable, Abstract, EditInlineNew, DefaultToInstanced)
class UBattleAbility : public UObject
{
    GENERATED_BODY()

public:
    /** 通用的技能激活接口（蓝图可重写） */
    UFUNCTION(BlueprintNativeEvent, Category = "Battle")
    void ActivateAbility(const FBattleAbilityContext& Context);
    virtual void ActivateAbility_Implementation(const FBattleAbilityContext& Context) {}
};
