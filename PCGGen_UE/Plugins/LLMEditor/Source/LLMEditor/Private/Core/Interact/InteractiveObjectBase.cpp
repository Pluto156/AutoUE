#include "InteractiveObjectBase.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#include "HealthBarUIComponent.h"
#include "BattleStatComponent.h"
#include "InteractiveCharacter.h"

#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

AInteractiveObjectBase::AInteractiveObjectBase()
{
    PrimaryActorTick.bCanEverTick = false;

    // -------------------------
    // Root
    // -------------------------
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // -------------------------
    // Static Mesh Component
    // -------------------------
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    MeshComponent->SetupAttachment(RootComponent);

    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Overlap);

    MeshComponent->OnComponentBeginOverlap.AddDynamic(
        this, &AInteractiveObjectBase::OnMeshBeginOverlap
    );

    // -------------------------
    // Skeletal Mesh Component
    // -------------------------
    SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
    SkeletalMeshComponent->SetupAttachment(RootComponent);

    SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SkeletalMeshComponent->SetGenerateOverlapEvents(false);

    // -------------------------
    // Box Collision
    // -------------------------
    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetupAttachment(RootComponent);
    BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // -------------------------
    // Battle & UI
    // -------------------------
    BattleStats = CreateDefaultSubobject<UBattleStatComponent>(TEXT("BattleStats"));
    HealthUIComponent = CreateDefaultSubobject<UHealthBarUIComponent>(TEXT("HealthUI"));

    bSnapToGround = true;
}

void AInteractiveObjectBase::BeginPlay()
{
    Super::BeginPlay();

    UpdateCollisionToMesh();

    if (bSnapToGround)
    {
        SnapToGround();
    }

    if (!bIsPartOfBattleSystem)
    {
        if (BattleStats)
        {
            BattleStats->SetComponentTickEnabled(false);
        }

        if (HealthUIComponent)
        {
            HealthUIComponent->HideUI();
        }

        UE_LOG(LogTemp, Log, TEXT("[%s] Not part of battle system."), *GetName());
    }
    else
    {
        if (HealthUIComponent)
        {
            HealthUIComponent->ShowUI();
        }

        if (BattleStats)
        {
            // 委托绑定 C++ 稳定接口
            BattleStats->OnDeath.AddDynamic(
                this, &AInteractiveObjectBase::OnDeathHandler
            );
        }

        UE_LOG(LogTemp, Log, TEXT("[%s] Registered into battle system."), *GetName());
    }

    SetupInteraction();
}

/**
 * Choose which mesh should drive bounds & placement
 */
static UPrimitiveComponent* GetPrimaryMesh(
    UStaticMeshComponent* StaticMesh,
    USkeletalMeshComponent* SkeletalMesh
)
{
    if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshAsset())
    {
        return SkeletalMesh;
    }
    return StaticMesh;
}

void AInteractiveObjectBase::UpdateCollisionToMesh()
{
    UPrimitiveComponent* PrimaryMesh =
        GetPrimaryMesh(MeshComponent, SkeletalMeshComponent);

    if (!PrimaryMesh || !BoxCollision)
    {
        return;
    }

    const FBoxSphereBounds Bounds = PrimaryMesh->Bounds;
    const FVector Extent = Bounds.BoxExtent;

    BoxCollision->SetBoxExtent(Extent);

    const FVector LocalCenter =
        PrimaryMesh->GetComponentTransform()
        .InverseTransformPosition(Bounds.Origin);

    BoxCollision->SetRelativeLocation(LocalCenter);
}

void AInteractiveObjectBase::SnapToGround()
{
    UPrimitiveComponent* PrimaryMesh =
        GetPrimaryMesh(MeshComponent, SkeletalMeshComponent);

    if (!PrimaryMesh)
    {
        return;
    }

    const FBoxSphereBounds Bounds = PrimaryMesh->Bounds;
    const float MeshMinZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;

    FVector Start(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z + 200.f);
    FVector End(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z - 2000.f);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_WorldStatic,
        Params
    );

    if (bHit)
    {
        const float DeltaZ = Hit.Location.Z - MeshMinZ;
        FVector NewLocation = GetActorLocation();
        NewLocation.Z += DeltaZ;
        SetActorLocation(NewLocation);

        UE_LOG(
            LogTemp,
            Log,
            TEXT("[%s] Snapped to ground. ΔZ=%.2f"),
            *GetName(),
            DeltaZ
        );
    }
}

void AInteractiveObjectBase::SetupInteraction_Implementation()
{
    OnInteract.AddDynamic(this, &AInteractiveObjectBase::HandleInteraction);
}

void AInteractiveObjectBase::CallOnInteract(
    AInteractiveCharacter* InteractingCharacter
)
{
    OnInteract.Broadcast(InteractingCharacter);
}


/* =====================================================
 * Interaction
 * ===================================================== */

void AInteractiveObjectBase::HandleInteraction(
    AInteractiveCharacter* InteractingCharacter
)
{
    // C++ 稳定入口 → 转发给 Blueprint 扩展点
    HandleInteraction_BP(InteractingCharacter);
}

void AInteractiveObjectBase::HandleInteraction_BP_Implementation(
    AInteractiveCharacter* InteractingCharacter
)
{
    const FString ObjName = GetName();
    const FString CharName =
        InteractingCharacter ? InteractingCharacter->GetName() : TEXT("Unknown");

    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    UE_LOG(
        LogTemp,
        Log,
        TEXT("%s was interacted by %s (default logic)."),
        *ObjName,
        *CharName
    );

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("%s was interacted by %s (default logic)."),
                *ObjName,
                *CharName
            )
        );
    }

}

void AInteractiveObjectBase::OnMeshBeginOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    if (!OtherActor)
    {
        return;
    }

    AInteractiveCharacter* PlayerCharacter =
        Cast<AInteractiveCharacter>(OtherActor);

    if (PlayerCharacter && PlayerCharacter->Tags.Contains("Player"))
    {
        UE_LOG(
            LogTemp,
            Log,
            TEXT("[%s] Player entered interaction range."),
            *GetName()
        );

        PlayerCharacter->TriggerInteract(this);
    }
}

/* =====================================================
 * Death
 * ===================================================== */

void AInteractiveObjectBase::OnDeathHandler(AActor* Killer)
{
    // C++ 稳定入口 → 转发给 Blueprint 扩展点
    OnDeathHandler_BP(Killer);
}

void AInteractiveObjectBase::OnDeathHandler_BP_Implementation(AActor* Killer)
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    UE_LOG(LogTemp, Warning, TEXT("%s has died"), *GetName());

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(TEXT("%s has died"), *GetName())
        );
    }

    Destroy();
}
