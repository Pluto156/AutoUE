#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "InteractiveObjectBase.generated.h"

class AInteractiveCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnInteractSignature,
    AInteractiveCharacter*, InteractingCharacter
);

UCLASS()
class AInteractiveObjectBase : public AActor, public IBattleInterface
{
    GENERATED_BODY()

public:
    AInteractiveObjectBase();

protected:
    virtual void BeginPlay() override;

    void UpdateCollisionToMesh();
    void SnapToGround();

protected:
    // ================= Components =================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    /** Skeletal mesh representation (optional, editable in Blueprint) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USkeletalMeshComponent* SkeletalMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* BoxCollision;

    // ================= Battle =================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle")
    bool bIsPartOfBattleSystem = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
    UBattleStatComponent* BattleStats;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UHealthBarUIComponent* HealthUIComponent;

    /**
     * Whether to snap this actor to ground on BeginPlay
     * Can be configured in Blueprint
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    bool bSnapToGround = true;

public:
    // ================= Battle Interface =================

    virtual UBattleStatComponent* GetBattleStatComponent_Implementation() const override
    {
        return BattleStats;
    }

public:
    // ================= Interaction =================

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractSignature OnInteract;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void CallOnInteract(AInteractiveCharacter* InteractingCharacter);


    /* ---------- Setup ---------- */

    UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
    void SetupInteraction();
    virtual void SetupInteraction_Implementation();

    /* ---------- Overlap ---------- */

    UFUNCTION()
    void OnMeshBeginOverlap(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    /* =====================================================
     * Interaction (C++ Stable Interface)
     * ❶ 所有已有 C++ 子类 override 的就是这个
     * ===================================================== */
    UFUNCTION()
    virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter);

    /* =====================================================
     * Interaction (Blueprint Extension Point)
     * ❷ 蓝图 Override 的是这个
     * ===================================================== */
    UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
    void HandleInteraction_BP(AInteractiveCharacter* InteractingCharacter);
    virtual void HandleInteraction_BP_Implementation(
        AInteractiveCharacter* InteractingCharacter
    );

    /* =====================================================
     * Death (C++ Stable Interface)
     * ===================================================== */
    UFUNCTION()
    virtual void OnDeathHandler(AActor* Killer);

    /* =====================================================
     * Death (Blueprint Extension Point)
     * ===================================================== */
    UFUNCTION(BlueprintNativeEvent, Category = "Battle")
    void OnDeathHandler_BP(AActor* Killer);
    virtual void OnDeathHandler_BP_Implementation(AActor* Killer);
};
