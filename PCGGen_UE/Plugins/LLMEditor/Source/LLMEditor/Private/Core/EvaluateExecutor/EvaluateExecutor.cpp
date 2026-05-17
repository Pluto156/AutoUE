// EvaluateExecutor.cpp

#include "EvaluateExecutor.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "NavigationSystem.h"
#include "HighResScreenshot.h"
#include <Runtime/AIModule/Classes/AIController.h>
#include "Navigation/PathFollowingComponent.h"
#include <CustomModules.h>
#include "Core/Interact/InteractiveObjectBase.h"
static const FString SaveDir =
TEXT("E:/UEProjects/ElectricDreamsEnv/Content/MyPCG/eval/");
AEvaluateExecutor::AEvaluateExecutor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AEvaluateExecutor::BeginPlay()
{
    Super::BeginPlay();
    SetupInputComponent();
    APawn* Pawn = GetControlledPawn();
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    //if (Pawn)
    //{
    //    if (PC && PC->GetPawn() == Pawn)
    //    {
    //        UE_LOG(LogTemp, Warning, TEXT("[EvaluateExecutor] PlayerController unpossess Pawn."));
    //        PC->UnPossess();
    //    }

    //    AController* ExistingController = Pawn->GetController();
    //    if (ExistingController && ExistingController->IsA<AAIController>())
    //    {
    //        ControlledAI = Cast<AAIController>(ExistingController);
    //    }
    //    else
    //    {
    //        AAIController* NewAI = GetWorld()->SpawnActor<AAIController>(
    //            AAIController::StaticClass(),
    //            Pawn->GetActorLocation(),
    //            Pawn->GetActorRotation()
    //        );

    //        if (NewAI)
    //        {
    //            NewAI->Possess(Pawn);
    //            ControlledAI = NewAI;

    //            PC->SetViewTarget(Pawn);
    //            if (PC->PlayerCameraManager)
    //            {
    //                PC->PlayerCameraManager->ViewTarget.Target = Pawn;
    //            }
    //        }
    //    }
    //}

    FString JsonPath =
        FPaths::ProjectContentDir() / TEXT("MyPCG/eval/instructions.json");

    if (!LoadInstructionJson(JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[EvaluateExecutor] Failed to load instructions.json"));
        return;
    }

    CurrentStepIndex = 0;

    GetWorldTimerManager().SetTimer(
        DelayStartHandle,
        this,
        &AEvaluateExecutor::ExecuteNextStep,
        1.0f,
        false
    );
}

// ================================================================
//                      解析 JSON
// ================================================================
bool AEvaluateExecutor::LoadInstructionJson(const FString& FilePath)
{
    FString JsonContent;

    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
        return false;

    TSharedPtr<FJsonObject> RootObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

    if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
        return false;

    return ParseInstructions(RootObj);
}

bool AEvaluateExecutor::ParseInstructions(const TSharedPtr<FJsonObject>& Root)
{
    const TArray<TSharedPtr<FJsonValue>>* ArrayPtr;

    if (!Root->TryGetArrayField(TEXT("evaluation_instructions"), ArrayPtr))
        return false;

    for (const auto& StepValue : *ArrayPtr)
    {
        const TSharedPtr<FJsonObject> Obj = StepValue->AsObject();
        if (!Obj.IsValid())
            continue;

        FEvaluateInstruction Inst;
        Inst.StepId = Obj->GetIntegerField("step_id");
        Inst.Action = Obj->GetStringField("action");
        Inst.Target = Obj->GetStringField("target");
        Inst.Description = Obj->GetStringField("description");

        Instructions.Add(Inst);
    }

    return true;
}

// ================================================================
//                截图辅助函数
// ================================================================
void AEvaluateExecutor::TakeStepScreenshot(const FString& FileName, TFunction<void()> Callback)
{
    FString FullPath = SaveDir + FileName + TEXT(".png");

    FScreenshotRequest::RequestScreenshot(FullPath, false, false);

    // 等待 0.2 秒让 UE 写完截图
    GetWorldTimerManager().SetTimer(
        ScreenshotHandle,
        FTimerDelegate::CreateLambda([Callback]() {
            Callback();
            }),
        0.2f,
        false
    );
}

// ================================================================
//                    执行步骤入口
// ================================================================
void AEvaluateExecutor::ExecuteNextStep()
{
    if (!Instructions.IsValidIndex(CurrentStepIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("[EvaluateExecutor] All steps done."));
        return;
    }

    const FEvaluateInstruction& Inst = Instructions[CurrentStepIndex];

    FString PreShotName = FString::Printf(TEXT("Step_%d_pre"), Inst.StepId);

    // ⭐新增：屏幕左上角打印开始
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 5.f, FColor::Yellow,
            FString::Printf(TEXT("[Begin Step %d] %s → %s"),
                Inst.StepId, *Inst.Action, *Inst.Target)
        );
    }

    if (Inst.Action == TEXT("move_to"))
    {
        OnStepCompleted();
        //ExecuteMoveTo(Inst.Target);
    }
    else if (Inst.Action == TEXT("interact"))
    {
        ExecuteInteract(Inst.Target);
    }
    else
    {
        OnStepCompleted();
    }
}

// ================================================================
//                完成步骤 → 截图 + 打印
// ================================================================
void AEvaluateExecutor::OnStepCompleted()
{
    //const FEvaluateInstruction& Inst = Instructions[CurrentStepIndex];
    //FString PostShotName = FString::Printf(TEXT("Step_%d_post"), Inst.StepId);

    //// ⭐新增：屏幕左上角打印结束
    //if (GEngine)
    //{
    //    GEngine->AddOnScreenDebugMessage(
    //        -1, 5.f, FColor::Green,
    //        FString::Printf(TEXT("[Finish Step %d] %s done"),
    //            Inst.StepId, *Inst.Action)
    //    );
    //}

    //// ⭐ 先截图，再进入下一步
    //TakeStepScreenshot(
    //    PostShotName,
    //    [this]() {
    //        CurrentStepIndex++;
    //        ExecuteNextStep();
    //    }
    //);
    CurrentStepIndex++;
    ExecuteNextStep();
}

// ================================================================
//                      MoveTo 执行（沿路程截图）
// ================================================================
APawn* AEvaluateExecutor::GetControlledPawn() const
{
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (APawn* PlayerPawn = PC->GetPawn())
            return PlayerPawn;
    }

    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        if (Cast<AAIController>((*It)->GetController()))
            return *It;
    }

    return nullptr;
}

AActor* AEvaluateExecutor::FindActorByName(const FString& Name) const
{
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        if (It->GetName().Contains(Name))
            return *It;

    return nullptr;
}

// 新增成员变量：检查截图点
struct FMoveScreenshotData
{
    FVector StartLocation;
    FVector TargetLocation;
    TArray<float> SnapPoints; // 比例 0~1
    TArray<bool> SnapDone;
    FEvaluateInstruction Instruction;
};

FMoveScreenshotData CurrentMoveData;
FTimerHandle MoveCheckHandle;

void AEvaluateExecutor::ExecuteMoveTo(const FString& Target)
{
    if (!ControlledAI || !ControlledAI->GetPawn())
    {
        OnStepCompleted();
        return;
    }

    APawn* Pawn = ControlledAI->GetPawn();

    FString SearchKey = FString::Printf(TEXT("BP_%s"), *Target);
    TArray<AActor*> Candidates;

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (It->GetName().Contains(SearchKey))
        {
            Candidates.Add(*It);
        }
    }

    if (Candidates.Num() == 0)
    {
        OnStepCompleted();
        return;
    }

    AActor* TargetActor = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

    FVector MoveLocation = TargetActor->GetActorLocation();

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys)
    {
        FNavLocation NavLoc;
        float Radius = 200.f;
        if (NavSys->GetRandomPointInNavigableRadius(TargetActor->GetActorLocation(), Radius, NavLoc))
        {
            MoveLocation = NavLoc.Location;
        }
    }

    FAIMoveRequest MoveReq;
    MoveReq.SetGoalLocation(MoveLocation);
    MoveReq.SetAcceptanceRadius(150.f);
    MoveReq.SetUsePathfinding(true);

    ControlledAI->ReceiveMoveCompleted.RemoveDynamic(
        this,
        &AEvaluateExecutor::OnAIMoveCompleted
    );
    ControlledAI->ReceiveMoveCompleted.AddDynamic(
        this,
        &AEvaluateExecutor::OnAIMoveCompleted
    );

    // 初始化移动截图数据
    CurrentMoveData.StartLocation = Pawn->GetActorLocation();
    CurrentMoveData.TargetLocation = MoveLocation;
    CurrentMoveData.SnapPoints = { 0.4f, 0.8f }; // 40%, 80%
    CurrentMoveData.SnapDone = { false, false };
    CurrentMoveData.Instruction = Instructions[CurrentStepIndex];

    // 启动定时检查移动进度
    GetWorldTimerManager().SetTimer(
        MoveCheckHandle,
        this,
        &AEvaluateExecutor::CheckMoveProgress,
        0.1f, // 每0.1秒检查
        true
    );

    auto Result = ControlledAI->MoveTo(MoveReq);

    UE_LOG(LogTemp, Warning,
        TEXT("[ExecuteMoveTo] MoveTo result: %d"),
        (int32)Result.Code
    );

    if (Result.Code == EPathFollowingRequestResult::Failed)
    {
        GetWorldTimerManager().ClearTimer(MoveCheckHandle);
        OnStepCompleted();
    }
}

void AEvaluateExecutor::CheckMoveProgress()
{
    if (!ControlledAI || !ControlledAI->GetPawn())
        return;

    FVector CurrentLoc = ControlledAI->GetPawn()->GetActorLocation();
    FVector Start = CurrentMoveData.StartLocation;
    FVector End = CurrentMoveData.TargetLocation;
    float TotalDist = FVector::Dist(Start, End);
    float CurrDist = FVector::Dist(Start, CurrentLoc);
    float Ratio = (TotalDist > 0.f) ? CurrDist / TotalDist : 1.f;

    for (int i = 0; i < CurrentMoveData.SnapPoints.Num(); ++i)
    {
        if (!CurrentMoveData.SnapDone[i] && Ratio >= CurrentMoveData.SnapPoints[i])
        {
            CurrentMoveData.SnapDone[i] = true;
            FString ShotName = FString::Printf(TEXT("Step_%d_mid_%d"), CurrentMoveData.Instruction.StepId, i + 1);
            TakeStepScreenshot(ShotName, []() {});
        }
    }
}

void AEvaluateExecutor::OnAIMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    GetWorldTimerManager().ClearTimer(MoveCheckHandle);

    if (ControlledAI)
        ControlledAI->ReceiveMoveCompleted.RemoveDynamic(this, &AEvaluateExecutor::OnAIMoveCompleted);

    OnStepCompleted();
}

// ================================================================
//                           Interact
// ================================================================
void AEvaluateExecutor::ExecuteInteract(const FString& Target)
{
    APawn* Pawn = GetControlledPawn();
    if (!Pawn)
    {
        OnStepCompleted();
        return;
    }

    // 当前控制的玩家角色（交互角色）
    AInteractiveCharacter* InteractingCharacter =
        Cast<AInteractiveCharacter>(Pawn);

    if (!InteractingCharacter)
    {
        OnStepCompleted();
        return;
    }

    const FString BPClassPrefix = TEXT("BP_") + Target;

    AInteractiveObjectBase* FoundTarget = nullptr;

    for (TActorIterator<AInteractiveObjectBase> It(GetWorld()); It; ++It)
    {
        AInteractiveObjectBase* Obj = *It;
        if (!Obj) continue;

        const FString ClassName = Obj->GetClass()->GetName();
        if (ClassName.StartsWith(BPClassPrefix))
        {
            FoundTarget = Obj;
            break;
        }
    }

    if (!FoundTarget)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ExecuteInteract] 未找到交互目标 BP_%s"),
            *Target);

        OnStepCompleted();
        return;
    }

    FoundTarget->HandleInteraction(InteractingCharacter);

    OnStepCompleted();
}

// ================================================================
//                          Input
// ================================================================
void AEvaluateExecutor::SetupInputComponent()
{
    EnableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));

    InputComp = InputComponent;
    if (!InputComp) return;

    InputComp->BindKey(
        EKeys::Q,
        IE_Pressed,
        this,
        &AEvaluateExecutor::OnPressScreenshotKey
    );
}

void AEvaluateExecutor::OnPressScreenshotKey()
{
    // 生成纯随机字符串（无前缀）
    const FString FileName =
        FGuid::NewGuid().ToString(EGuidFormats::Digits);

    TakeScreenshot(FileName);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.f,
            FColor::Cyan,
            TEXT("[Screenshot] Q pressed")
        );
    }
}



void AEvaluateExecutor::TakeScreenshot(const FString& FileName)
{
    FString FullPath = SaveDir + FileName + TEXT(".png");
    FScreenshotRequest::RequestScreenshot(FullPath, false, false);
}