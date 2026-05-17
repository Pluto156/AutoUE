#pragma once

#include "PCGSettings.h"
#include "Engine/DataAsset.h"
#include "PCGPromptSpatialDistribution.generated.h"

UENUM(BlueprintType)
enum class EPCGDirectionSlot : uint8
{
	Left,
	Right,
	Front,
	Back,
	Top,
	Bottom,
	Surround
};

USTRUCT(BlueprintType)
struct FPCGSpatialRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	EPCGDirectionSlot Slot = EPCGDirectionSlot::Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	TSoftObjectPtr<AActor> BaseActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	FVector Extent = FVector(200, 200, 100);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule", meta = (ClampMin = "0"))
	int32 Count = 10;
};

 
UCLASS(BlueprintType)
class UPCGSpatialRuleAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG")
	TArray<FPCGSpatialRule> Rules;
};

UCLASS(MinimalAPI, BlueprintType, ClassGroup = (Procedural))
class UPCGPromptSpatialDistributionSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("PromptSpatialDistribution")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGPromptSpatialDistributionSettings", "NodeTitle", "Prompt Spatial Distribution"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::PointOps; }
#endif

	virtual bool UseSeed() const override { return true; }

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return Super::DefaultPointInputPinProperties(); }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UPCGSpatialRuleAsset> RuleAsset;
};

class FPCGPromptSpatialDistributionElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
	virtual EPCGElementExecutionLoopMode ExecutionLoopMode(const UPCGSettings* Settings) const override { return EPCGElementExecutionLoopMode::SinglePrimaryPin; }
	virtual bool SupportsBasePointDataInputs(FPCGContext* InContext) const override { return true; }
};
