#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleInterface.h"
#include "ProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class AProjectileBase : public AActor
{
    GENERATED_BODY()

public:
    AProjectileBase();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void InitTarget(AActor* InTarget);

protected:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float Speed = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float Damage = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float LifeTime = 5.f;

    UPROPERTY(BlueprintReadOnly, Category = "Projectile")
    AActor* TargetActor = nullptr;

    /* 命中事件（可在蓝图中重写） */
    UFUNCTION(BlueprintNativeEvent)
    void OnProjectileHit(AActor* OtherActor, const FHitResult& Hit);
    virtual void OnProjectileHit_Implementation(AActor* OtherActor, const FHitResult& Hit);

    /* 组件 Hit 回调 */
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse,
        const FHitResult& Hit);
};
