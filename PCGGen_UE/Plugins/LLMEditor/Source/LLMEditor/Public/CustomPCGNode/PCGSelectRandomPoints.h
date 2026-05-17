#pragma once

#include "PCGSettings.h"
#include "PCGSelectRandomPoints.generated.h"

UENUM(BlueprintType)
enum class EPCGSelectRandomMode : uint8
{
	ByCount UMETA(DisplayName = "Select by Count"),
	ByRatio UMETA(DisplayName = "Select by Ratio")
};

UCLASS(MinimalAPI, BlueprintType, ClassGroup = (Procedural))
class UPCGSelectRandomPointsSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("SelectRandomPoints")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGSelectRandomPointsSettings", "NodeTitle", "Select Random Points"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::PointOps; }
#endif
	virtual bool UseSeed() const override { return true; }

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return Super::DefaultPointInputPinProperties();
	}
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override
	{
		TArray<FPCGPinProperties> Pins;
		Pins.Add(FPCGPinProperties(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point));       // Selected Points
		Pins.Add(FPCGPinProperties(TEXT("RemainingPoints"), EPCGDataType::Point));                     // Remaining Points
		return Pins;
	}
	virtual FPCGElementPtr CreateElement() const override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	EPCGSelectRandomMode SelectMode = EPCGSelectRandomMode::ByCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "SelectMode == EPCGSelectRandomMode::ByCount", ClampMin = "1", PCG_Overridable))
	int32 NumPoints = 100;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "SelectMode == EPCGSelectRandomMode::ByRatio", ClampMin = "0.0", ClampMax = "1.0", PCG_Overridable))
	float Ratio = 0.1f;
};

class FPCGSelectRandomPointsElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
	virtual EPCGElementExecutionLoopMode ExecutionLoopMode(const UPCGSettings* Settings) const override { return EPCGElementExecutionLoopMode::SinglePrimaryPin; }
	virtual bool SupportsBasePointDataInputs(FPCGContext* InContext) const override { return true; }
};
