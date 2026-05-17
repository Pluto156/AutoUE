#pragma once

#include "CoreMinimal.h"
#include "ThirdPersonTemplateCharacter.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "InputAction.h"
#include "HealthBarUIComponent.h"
#include "InteractiveCharacter.generated.h"

class UAnimInstance;
class UInputAction;
class AInteractiveObjectBase;

/** 战斗姿态 */
UENUM(BlueprintType)
enum class ECombatState : uint8
{
    Unarmed UMETA(DisplayName = "Unarmed"),
    Armed UMETA(DisplayName = "Armed")
};

UCLASS()
class AInteractiveCharacter
    : public AThirdPersonTemplateCharacter,
    public IBattleInterface
{
    GENERATED_BODY()

public:
    AInteractiveCharacter();

    /** 交互 */
    UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
    void TriggerInteract(AInteractiveObjectBase* InteractiveObject);
    virtual void TriggerInteract_Implementation(AInteractiveObjectBase* InteractiveObject);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
    UBattleStatComponent* BattleStats;

    // 头顶血条组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UHealthBarUIComponent* HealthUIComponent;


    /** 鼠标滚轮 Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* MouseScrollAction;

    /** 当前战斗姿态 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    ECombatState CombatState;

    /** 徒手动画 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    TSubclassOf<UAnimInstance> UnarmedAnimBP;

    /** 持枪动画 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    TSubclassOf<UAnimInstance> ArmedAnimBP;

protected:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    /** 鼠标滚轮回调 */
    void MouseScroll(const FInputActionValue& Value);

    /** 切换战斗姿态 */
    void SwitchCombatState(ECombatState NewState);

public:
    virtual UBattleStatComponent* GetBattleStatComponent_Implementation() const override
    {
        return BattleStats;
    }
};
