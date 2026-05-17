#include "ProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "BattleStatComponent.h"

AProjectileBase::AProjectileBase()
{
    PrimaryActorTick.bCanEverTick = false;

    /* =============================
       Sphere Collision（Hit 模式）
       ============================= */
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    RootComponent = CollisionComp;

    CollisionComp->InitSphereRadius(10.f);

    // ✔ 启用阻挡、Hit 事件
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel1);
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

    CollisionComp->SetNotifyRigidBodyCollision(true); // 必须：使 Hit 生效

    // ✔ 注册 Hit 回调
    CollisionComp->OnComponentHit.AddDynamic(this, &AProjectileBase::OnHit);

    /* =============================
       Mesh（仅显示：完全无碰撞）
       ============================= */
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    /* =============================
       Projectile Movement
       ============================= */
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = Speed;
    ProjectileMovement->MaxSpeed = Speed;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.f;

    // ✔ 防止高速穿透
    ProjectileMovement->bSweepCollision = true;

    InitialLifeSpan = LifeTime;
}

void AProjectileBase::BeginPlay()
{
    Super::BeginPlay();
}

void AProjectileBase::InitTarget(AActor* InTarget)
{
    TargetActor = InTarget;
    if (TargetActor)
    {
        FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        ProjectileMovement->Velocity = Direction * Speed;
    }
}

void AProjectileBase::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    // 调用 BlueprintNativeEvent（蓝图能重写）
    OnProjectileHit(OtherActor, Hit);
}

/* =============================
   BlueprintNativeEvent 默认实现
   蓝图可 Call Parent
   ============================= */
void AProjectileBase::OnProjectileHit_Implementation(AActor* OtherActor, const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
        return;

    // 调用接口伤害
    if (OtherActor->Implements<UBattleInterface>())
    {
        UBattleStatComponent* StatComp =
            IBattleInterface::Execute_GetBattleStatComponent(OtherActor);

        if (StatComp)
        {
            StatComp->ApplyDamage(Damage, GetOwner());
        }
    }

    Destroy(); // 命中后销毁
}
