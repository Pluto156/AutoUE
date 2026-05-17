#include "HealthBarUIComponent.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"

#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

UHealthBarUIComponent::UHealthBarUIComponent()
{
    PrimaryComponentTick.bCanEverTick = true;   // ⭐ 必须开启 Tick 以实现跟随摄像机旋转
}

void UHealthBarUIComponent::BeginPlay()
{
    Super::BeginPlay();

    // 1. 获取战斗组件
    if (AActor* Owner = GetOwner())
    {
        if (Owner->Implements<UBattleInterface>())
        {
            StatComp = IBattleInterface::Execute_GetBattleStatComponent(Owner);
        }
    }

    if (!StatComp)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("NULL");

        UE_LOG(LogTemp, Warning,
            TEXT("[HealthBarUI] No BattleStatComponent found. Owner = %s"),
            *OwnerName);

        return;
    }


    // 2. 创建 UI 组件
    InitUI();

    // 3. 绑定事件
    StatComp->OnHealthChanged.AddDynamic(this, &UHealthBarUIComponent::OnHealthChanged);

    // 4. 初始化 UI
    OnHealthChanged();
}

void UHealthBarUIComponent::InitUI()
{
    if (!HealthBarWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[HealthBarUI] HealthBarWidgetClass not set!"));
        return;
    }

    // 创建 WidgetComponent
    HealthBarWidget = NewObject<UWidgetComponent>(GetOwner(), TEXT("HealthBarWidget"));
    HealthBarWidget->RegisterComponent();
    HealthBarWidget->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

    HealthBarWidget->SetWidgetSpace(EWidgetSpace::World);
    HealthBarWidget->SetDrawSize(HealthBarDrawSize);
    HealthBarWidget->SetRelativeLocation(WidgetOffset);


    // 禁止碰撞
    HealthBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 设置 UI
    HealthBarWidget->SetWidgetClass(HealthBarWidgetClass);
    HealthBarWidget->InitWidget();

    HealthBarUserWidget = HealthBarWidget->GetUserWidgetObject();
}

void UHealthBarUIComponent::OnHealthChanged()
{
    if (!StatComp || !HealthBarUserWidget)
        return;

    if (UFunction* Func = HealthBarUserWidget->FindFunction(TEXT("UpdateHealth")))
    {
        uint8* Buffer = (uint8*)FMemory::Malloc(Func->ParmsSize);
        FMemory::Memzero(Buffer, Func->ParmsSize);

        float Current = StatComp->GetHealth();
        float Max = StatComp->GetMaxHealth();

        for (TFieldIterator<FProperty> It(Func); It && (It->PropertyFlags & CPF_Parm); ++It)
        {
            FProperty* Prop = *It;

            if (Prop->GetName().Equals(TEXT("CurrentHealth")))
            {
                if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
                {
                    FloatProp->SetPropertyValue_InContainer(Buffer, Current);
                }
                else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
                {
                    DoubleProp->SetPropertyValue_InContainer(Buffer, (double)Current);
                }
            }
            else if (Prop->GetName().Equals(TEXT("MaxHealth")))
            {
                if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
                {
                    FloatProp->SetPropertyValue_InContainer(Buffer, Max);
                }
                else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
                {
                    DoubleProp->SetPropertyValue_InContainer(Buffer, (double)Max);
                }
            }
        }

        HealthBarUserWidget->ProcessEvent(Func, Buffer);
        FMemory::Free(Buffer);
    }
}


void UHealthBarUIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateFacingCamera();
}

void UHealthBarUIComponent::UpdateFacingCamera()
{
    if (!HealthBarWidget)
        return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    APlayerCameraManager* CamMgr = PC->PlayerCameraManager;
    if (!CamMgr) return;

    FVector CamLoc = CamMgr->GetCameraLocation();

    FVector WidgetLoc = HealthBarWidget->GetComponentLocation();

    // 让 WidgetComponent 朝向摄像机
    FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(WidgetLoc, CamLoc);

    // 保持血条只旋转 Yaw（水平），不上下翻转
    LookAtRot.Pitch = 0.f;
    LookAtRot.Roll = 0.f;

    HealthBarWidget->SetWorldRotation(LookAtRot);
}


void UHealthBarUIComponent::ShowUI()
{
    if (HealthBarWidget)
    {
        HealthBarWidget->SetHiddenInGame(false); // 显示 WidgetComponent
    }

    if (HealthBarUserWidget)
    {
        HealthBarUserWidget->SetVisibility(ESlateVisibility::Visible); // 显示 UI 元素
    }
}

void UHealthBarUIComponent::HideUI()
{
    if (HealthBarWidget)
    {
        HealthBarWidget->SetHiddenInGame(true); // 隐藏 WidgetComponent
    }

    if (HealthBarUserWidget)
    {
        HealthBarUserWidget->SetVisibility(ESlateVisibility::Collapsed); // 隐藏 UI 元素
    }
}