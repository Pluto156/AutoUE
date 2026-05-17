#include "BattleStatComponent.h"
#include "GameFramework/Actor.h"
#include "BattleAbility.h"

UBattleStatComponent::UBattleStatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CurrentHealth = MaxHealth;
}

void UBattleStatComponent::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
    if (DamageAmount <= 0.f || IsDead())
        return;

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

    UE_LOG(LogTemp, Log, TEXT("[%s] took %.1f damage (%.1f / %.1f)"),
        *GetOwner()->GetName(), DamageAmount, CurrentHealth, MaxHealth);

    // 广播血量变化事件（UI 等可绑定）
    OnHealthChanged.Broadcast();

    if (IsDead())
    {
        OnDeath.Broadcast(DamageCauser);
        UE_LOG(LogTemp, Warning, TEXT("[%s] died!"), *GetOwner()->GetName());
    }
}

void UBattleStatComponent::ResetHealth()
{
    CurrentHealth = MaxHealth;

    // 广播血量变化事件（UI 等可绑定）
    OnHealthChanged.Broadcast();
}

void UBattleStatComponent::InitializeAbilities()
{
    AbilityInstances.Empty();

    for (TSubclassOf<UBattleAbility> AbilityClass : AbilityClasses)
    {
        if (!AbilityClass)
            continue;

        UBattleAbility* NewAbility = NewObject<UBattleAbility>(this, AbilityClass);

        if (NewAbility)
        {
            AbilityInstances.Add(NewAbility);
            UE_LOG(LogTemp, Log, TEXT("[BattleStatComponent] Ability created: %s"),
                *AbilityClass->GetName());
        }
    }
}

void UBattleStatComponent::UseAbility(int32 AbilityIndex, const FBattleAbilityContext& Context)
{
    if (AbilityInstances.IsValidIndex(AbilityIndex) && AbilityInstances[AbilityIndex])
    {
        AbilityInstances[AbilityIndex]->ActivateAbility(Context);
    }
}
