// EvaluateExecutor.h

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Navigation/PathFollowingComponent.h"
#include <Runtime/AIModule/Classes/AIController.h>
#include "EvaluateExecutor.generated.h"

USTRUCT()
struct FEvaluateInstruction
{
    GENERATED_BODY()

    UPROPERTY()
    int32 StepId;

    UPROPERTY()
    FString Action;

    UPROPERTY()
    FString Target;

    UPROPERTY()
    FString Description;
};


UCLASS()
class AEvaluateExecutor : public AActor
{
    GENERATED_BODY()

public:
    AEvaluateExecutor();

protected:
    virtual void BeginPlay() override;

    bool LoadInstructionJson(const FString& FilePath);
    bool ParseInstructions(const TSharedPtr<FJsonObject>& Root);
    void CheckMoveProgress();
    void ExecuteNextStep();
    void OnStepCompleted();

    void ExecuteMoveTo(const FString& Target);
    void ExecuteInteract(const FString& Target);

    UFUNCTION()
    void OnAIMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

    AActor* FindActorByName(const FString& Name) const;
    APawn* GetControlledPawn() const;

    // ⭐ 新增：截图函数

    void TakeScreenshot(const FString& FileName);
    void TakeStepScreenshot(const FString& FileName, TFunction<void()> Callback);
    virtual void SetupInputComponent();
    // Q key handler
    void OnPressScreenshotKey();

private:
    UPROPERTY()
    TArray<FEvaluateInstruction> Instructions;

    int32 CurrentStepIndex = 0;

    FTimerHandle DelayStartHandle;
    FTimerHandle ScreenshotHandle;

    AAIController* ControlledAI = nullptr;
    UPROPERTY()
    UInputComponent* InputComp = nullptr;
};
