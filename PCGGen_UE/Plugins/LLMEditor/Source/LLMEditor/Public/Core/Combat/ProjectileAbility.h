#pragma once

#include "CoreMinimal.h"
#include "BattleAbility.h"
#include "ProjectileAbility.generated.h"

class AProjectileBase;

/**
 * 发射投射物的通用能力
 */
UCLASS()
class UProjectileAbility : public UBattleAbility
{
    GENERATED_BODY()

public:
    /** 要生成的投射物类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    TSubclassOf<AProjectileBase> ProjectileClass;

    /** 发射位置偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    FVector MuzzleOffset = FVector(100.f, 0.f, 50.f);

    /** 能力实现 */
    virtual void ActivateAbility_Implementation(const FBattleAbilityContext& Context) override;
};
