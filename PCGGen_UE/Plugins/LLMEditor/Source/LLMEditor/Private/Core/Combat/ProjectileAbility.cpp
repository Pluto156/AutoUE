#include "ProjectileAbility.h"
#include "ProjectileBase.h"
#include "Kismet/GameplayStatics.h"

void UProjectileAbility::ActivateAbility_Implementation(const FBattleAbilityContext& Context)
{
    if (!ProjectileClass || !Context.InstigatorActor)
        return;

    UWorld* World = Context.InstigatorActor->GetWorld();
    if (!World)
        return;

    // 默认使用角色的朝向，除非 Context 提供了自定义方向
    FVector ForwardDir = Context.Direction.IsNearlyZero()
        ? Context.InstigatorActor->GetActorForwardVector()
        : Context.Direction.GetSafeNormal();

    FVector SpawnLoc = Context.InstigatorActor->GetActorLocation() +
        ForwardDir * MuzzleOffset.X +
        FVector(0.f, 0.f, MuzzleOffset.Z);

    FRotator SpawnRot = ForwardDir.Rotation();

    FActorSpawnParameters Params;
    Params.Owner = Context.InstigatorActor;
    Params.Instigator = Cast<APawn>(Context.InstigatorActor);

    // 生成投射物
    AProjectileBase* Projectile = World->SpawnActor<AProjectileBase>(
        ProjectileClass, SpawnLoc, SpawnRot, Params);

    if (Projectile && Context.TargetActor)
    {
        // 如果有目标（例如锁定攻击），初始化目标信息
        Projectile->InitTarget(Context.TargetActor);
    }

    UE_LOG(LogTemp, Log, TEXT("[ProjectileAbility] %s fired projectile %s"),
        *Context.InstigatorActor->GetName(),
        Projectile ? *Projectile->GetName() : TEXT("None"));
}
