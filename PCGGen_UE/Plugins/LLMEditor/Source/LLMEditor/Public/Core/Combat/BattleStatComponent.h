#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleStatComponent.generated.h"

class UBattleAbility;
struct FBattleAbilityContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathSignature, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthChangedSignature);

UCLASS(ClassGroup = (Battle), meta = (BlueprintSpawnableComponent))
class UBattleStatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBattleStatComponent();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Stats")
    float MaxHealth = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Stats")
    float CurrentHealth;

public:
    /** 死亡事件（蓝图可绑定） */
    UPROPERTY(BlueprintAssignable, Category = "Battle Events")
    FOnDeathSignature OnDeath;

    /** 血量变化事件（蓝图可绑定，例如 UI 更新） */
    UPROPERTY(BlueprintAssignable, Category = "Battle Events")
    FOnHealthChangedSignature OnHealthChanged;

public:
    UFUNCTION(BlueprintCallable, Category = "Battle")
    float GetHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintCallable, Category = "Battle")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void ApplyDamage(float DamageAmount, AActor* DamageCauser);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void ResetHealth();

    UFUNCTION(BlueprintPure, Category = "Battle")
    bool IsDead() const { return CurrentHealth <= 0.f; }

    /** 能力类列表（蓝图可编辑） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UBattleAbility>> AbilityClasses;

private:
    /** 运行时实例，不暴露给蓝图 */
    UPROPERTY(Transient)
    TArray<UBattleAbility*> AbilityInstances;

public:
    /** 初始化能力实例 */
    UFUNCTION(BlueprintCallable, Category = "Battle|Ability")
    void InitializeAbilities();

    /** 使用指定编号的能力 */
    UFUNCTION(BlueprintCallable, Category = "Battle|Ability")
    void UseAbility(int32 AbilityIndex, const FBattleAbilityContext& Context);
};
