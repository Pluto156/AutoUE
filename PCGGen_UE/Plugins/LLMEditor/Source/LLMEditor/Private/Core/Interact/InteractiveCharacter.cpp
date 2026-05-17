#include "InteractiveCharacter.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "InteractiveObjectBase.h"
#include "ProjectileAbility.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

void AInteractiveCharacter::TriggerInteract_Implementation(AInteractiveObjectBase* InteractiveObject)
{
    if (GEngine)
    {
        FString ObjName = InteractiveObject ? InteractiveObject->GetName() : TEXT("Unknown");
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            FString::Printf(TEXT("Interacting with: %s"), *ObjName));
    }

    if (InteractiveObject)
    {
        InteractiveObject->OnInteract.Broadcast(this);
    }
}


AInteractiveCharacter::AInteractiveCharacter()
{
    BattleStats = CreateDefaultSubobject<UBattleStatComponent>(TEXT("BattleStats"));
    HealthUIComponent = CreateDefaultSubobject<UHealthBarUIComponent>(TEXT("HealthUI"));
    CombatState = ECombatState::Unarmed;
}

void AInteractiveCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInput->BindAction(MouseScrollAction, ETriggerEvent::Triggered,
            this, &AInteractiveCharacter::MouseScroll);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Enhanced Input not found on %s"), *GetNameSafe(this));
    }
}

void AInteractiveCharacter::MouseScroll(const FInputActionValue& Value)
{
    float ScrollValue = Value.Get<float>();

    if (ScrollValue > 0.f)   // 向上滚
    {
        SwitchCombatState(ECombatState::Armed);
    }
    else if (ScrollValue < 0.f)  // 向下滚
    {
        SwitchCombatState(ECombatState::Unarmed);
    }
}

void AInteractiveCharacter::SwitchCombatState(ECombatState NewState)
{
    if (CombatState == NewState)
        return;

    CombatState = NewState;

    // === 动画蓝图切换 ===
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp)
        return;

    switch (CombatState)
    {
    case ECombatState::Unarmed:
        if (UnarmedAnimBP)
        {
            MeshComp->SetAnimInstanceClass(UnarmedAnimBP);
        }
        break;

    case ECombatState::Armed:
        if (ArmedAnimBP)
        {
            MeshComp->SetAnimInstanceClass(ArmedAnimBP);
        }
        break;
    }

    // Debug
    if (GEngine)
    {
        FString StateName = (CombatState == ECombatState::Armed) ?
            TEXT("Armed (持枪)") : TEXT("Unarmed (徒手)");

        GEngine->AddOnScreenDebugMessage(
            -1, 1.5f, FColor::Green,
            FString::Printf(TEXT("Switched to: %s"), *StateName)
        );
    }
}
