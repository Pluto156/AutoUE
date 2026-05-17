#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthBarUIComponent.generated.h"

class UBattleStatComponent;
class UWidgetComponent;
class UUserWidget;

UCLASS(ClassGroup = (Battle), meta = (BlueprintSpawnableComponent))
class UHealthBarUIComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHealthBarUIComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void InitUI();
    void UpdateFacingCamera();

public:
    // 显示血条
    UFUNCTION(BlueprintCallable, Category = "Health Bar UI")
    void ShowUI();

    // 隐藏血条
    UFUNCTION(BlueprintCallable, Category = "Health Bar UI")
    void HideUI();

protected:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar UI")
    FVector WidgetOffset = FVector(0.f, 0.f, 120.f);
    /** 血条在世界中的尺寸（像素单位） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar UI")
    FVector2D HealthBarDrawSize = FVector2D(150.f, 20.f);
    /** 血条 UI 蓝图类 */
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> HealthBarWidgetClass;

    /** WidgetComponent */
    UPROPERTY(Transient)
    UWidgetComponent* HealthBarWidget = nullptr;

    /** 创建出的 UserWidget */
    UPROPERTY(Transient)
    UUserWidget* HealthBarUserWidget = nullptr;

    /** 战斗属性组件 */
    UPROPERTY()
    UBattleStatComponent* StatComp = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Health")
    void OnHealthChanged();

};
