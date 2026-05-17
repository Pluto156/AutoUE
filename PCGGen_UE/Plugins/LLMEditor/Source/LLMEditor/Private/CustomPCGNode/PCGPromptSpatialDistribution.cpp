#include "PCGPromptSpatialDistribution.h"
#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGBasePointData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"
#include "Math/RandomStream.h"

#define LOCTEXT_NAMESPACE "PCGPromptSpatialDistribution"

TArray<FPCGPinProperties> UPCGPromptSpatialDistributionSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Pins;
	UEnum* EnumPtr = StaticEnum<EPCGDirectionSlot>();
	if (!EnumPtr) return Pins;

	for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
	{
		Pins.Add(FPCGPinProperties(*EnumPtr->GetNameStringByIndex(i), EPCGDataType::Point));
	}
	return Pins;
}

FPCGElementPtr UPCGPromptSpatialDistributionSettings::CreateElement() const
{
	return MakeShared<FPCGPromptSpatialDistributionElement>();
}

static FVector RandomPointInDirectionBox(
	EPCGDirectionSlot Slot,
	const FVector& Offset,
	const FVector& Extent,
	FRandomStream& Random)
{
	FVector Point = Offset;

	switch (Slot)
	{
	case EPCGDirectionSlot::Left:
		Point.X -= Random.FRandRange(0.f, Extent.X);
		Point.Y += Random.FRandRange(-Extent.Y, Extent.Y);
		Point.Z += Random.FRandRange(-Extent.Z, Extent.Z);
		break;

	case EPCGDirectionSlot::Right:
		Point.X += Random.FRandRange(0.f, Extent.X);
		Point.Y += Random.FRandRange(-Extent.Y, Extent.Y);
		Point.Z += Random.FRandRange(-Extent.Z, Extent.Z);
		break;

	case EPCGDirectionSlot::Front:
		Point.Y += Random.FRandRange(0.f, Extent.Y);
		Point.X += Random.FRandRange(-Extent.X, Extent.X);
		Point.Z += Random.FRandRange(-Extent.Z, Extent.Z);
		break;

	case EPCGDirectionSlot::Back:
		Point.Y -= Random.FRandRange(0.f, Extent.Y);
		Point.X += Random.FRandRange(-Extent.X, Extent.X);
		Point.Z += Random.FRandRange(-Extent.Z, Extent.Z);
		break;

	case EPCGDirectionSlot::Top:
		Point.Z += Random.FRandRange(0.f, Extent.Z);
		Point.X += Random.FRandRange(-Extent.X, Extent.X);
		Point.Y += Random.FRandRange(-Extent.Y, Extent.Y);
		break;

	case EPCGDirectionSlot::Bottom:
		Point.Z -= Random.FRandRange(0.f, Extent.Z);
		Point.X += Random.FRandRange(-Extent.X, Extent.X);
		Point.Y += Random.FRandRange(-Extent.Y, Extent.Y);
		break;

	case EPCGDirectionSlot::Surround:
	default:
		Point.X += Random.FRandRange(-Extent.X, Extent.X);
		Point.Y += Random.FRandRange(-Extent.Y, Extent.Y);
		Point.Z += Random.FRandRange(-Extent.Z, Extent.Z);
		break;
	}

	return Point;
}

bool FPCGPromptSpatialDistributionElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGPromptSpatialDistributionElement::Execute);

	const UPCGPromptSpatialDistributionSettings* Settings =
		Context->GetInputSettings<UPCGPromptSpatialDistributionSettings>();
	check(Settings);

	if (!Settings->RuleAsset)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("MissingRuleAsset", "RuleAsset is not assigned"));
		return true;
	}

	const TArray<FPCGSpatialRule>& Rules = Settings->RuleAsset->Rules;

	const TArray<FPCGTaggedData> Inputs =
		Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);

	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
	const int Seed = Context->GetSeed();

	for (const FPCGTaggedData& Input : Inputs)
	{
		const UPCGSpatialData* SpatialData = Cast<UPCGSpatialData>(Input.Data);
		if (!SpatialData) continue;

		const UPCGBasePointData* PointData = SpatialData->ToBasePointData(Context);
		if (!PointData || PointData->GetNumPoints() == 0) continue;

		FRandomStream RandomStream(Seed);

		TMap<EPCGDirectionSlot, UPCGBasePointData*> SlotOutputs;

		for (const FPCGSpatialRule& Rule : Rules)
		{
			if (!SlotOutputs.Contains(Rule.Slot))
			{
				UPCGBasePointData* NewData = FPCGContext::NewPointData_AnyThread(Context);
				NewData->AllocateProperties(PointData->GetAllocatedProperties());
				NewData->SetNumPoints(0);

				SlotOutputs.Add(Rule.Slot, NewData);

				FPCGTaggedData& OutData = Outputs.Add_GetRef(Input);
				OutData.Data = NewData;
				OutData.Pin = FName(*UEnum::GetValueAsString(Rule.Slot).RightChop(FString("EPCGDirectionSlot::").Len()));
			}
		}

		for (const FPCGSpatialRule& Rule : Rules)
		{
			UPCGBasePointData* OutputData = SlotOutputs[Rule.Slot];

			const int32 StartIndex = OutputData->GetNumPoints();
			OutputData->SetNumPoints(StartIndex + Rule.Count, false);

			const TConstPCGValueRange<FTransform> InputTransforms =
				PointData->GetConstTransformValueRange();
			TPCGValueRange<FTransform> OutputTransforms =
				OutputData->GetTransformValueRange(true);

			for (int32 i = 0; i < Rule.Count; ++i)
			{
				FVector BaseLocation = Rule.BaseActor.IsValid()
					? Rule.BaseActor->GetActorLocation()
					: InputTransforms[RandomStream.RandRange(0, InputTransforms.Num() - 1)].GetLocation();

				FVector FinalLocation =
					RandomPointInDirectionBox(
						Rule.Slot,
						BaseLocation + Rule.Offset,
						Rule.Extent,
						RandomStream);

				OutputTransforms[StartIndex + i] = FTransform(FinalLocation);
			}
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
