// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCGSelectRandomPoints.h"
#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGBasePointData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"
#include "Math/RandomStream.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PCGSelectRandomPoints)

#define LOCTEXT_NAMESPACE "PCGSelectRandomPoints"

FPCGElementPtr UPCGSelectRandomPointsSettings::CreateElement() const
{
	return MakeShared<FPCGSelectRandomPointsElement>();
}

bool FPCGSelectRandomPointsElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGSelectRandomPointsElement::Execute);

	const UPCGSelectRandomPointsSettings* Settings = Context->GetInputSettings<UPCGSelectRandomPointsSettings>();
	check(Settings);

	const TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	const int Seed = Context->GetSeed();

	for (const FPCGTaggedData& Input : Inputs)
	{
		const UPCGSpatialData* SpatialData = Cast<UPCGSpatialData>(Input.Data);
		if (!SpatialData)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("MissingSpatialData", "Input does not contain spatial data"));
			continue;
		}

		const UPCGBasePointData* PointData = SpatialData->ToBasePointData(Context);
		if (!PointData)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("MissingPointData", "Input does not contain point data"));
			continue;
		}

		const int32 TotalPoints = PointData->GetNumPoints();
		if (TotalPoints == 0)
		{
			PCGE_LOG(Warning, GraphAndLog, LOCTEXT("NoPoints", "Input has no points"));
			continue;
		}

		int32 NumToSelect = (Settings->SelectMode == EPCGSelectRandomMode::ByCount)
			? FMath::Clamp(Settings->NumPoints, 0, TotalPoints)
			: FMath::Clamp(FMath::RoundToInt(TotalPoints * Settings->Ratio), 0, TotalPoints);

		// ---- 创建输出对象 ----
		UPCGBasePointData* SelectedData = FPCGContext::NewPointData_AnyThread(Context);
		SelectedData->InitializeFromData(PointData);
		FPCGTaggedData& SelectedOutput = Outputs.Add_GetRef(Input);
		SelectedOutput.Pin = PCGPinConstants::DefaultOutputLabel;
		SelectedOutput.Data = SelectedData;

		UPCGBasePointData* RemainingData = FPCGContext::NewPointData_AnyThread(Context);
		RemainingData->InitializeFromData(PointData);
		FPCGTaggedData& RemainingOutput = Outputs.Add_GetRef(Input);
		RemainingOutput.Pin = TEXT("RemainingPoints");
		RemainingOutput.Data = RemainingData;

		// ---- 特殊情况: 不选中任何点 ----
		if (NumToSelect == 0)
		{
			SelectedData->SetNumPoints(0);
			RemainingData->SetNumPoints(TotalPoints);
			continue;
		}

		// ---- 打乱索引 ----
		TArray<int32> Indices;
		Indices.Reserve(TotalPoints);
		for (int32 i = 0; i < TotalPoints; ++i)
		{
			Indices.Add(i);
		}

		FRandomStream RandomStream(Seed);
		for (int32 i = TotalPoints - 1; i > 0; --i)
		{
			int32 j = RandomStream.RandRange(0, i);
			Indices.Swap(i, j);
		}

		// ---- 分配 Selected / Remaining 索引 ----
		TArray<int32> SelectedIndices(Indices.GetData(), NumToSelect);
		TArray<int32> RemainingIndices;
		for (int32 i = NumToSelect; i < Indices.Num(); ++i)
		{
			RemainingIndices.Add(Indices[i]);
		}

		// ---- 分配输出空间 ----
		SelectedData->SetNumPoints(NumToSelect, /*bInitializeValues=*/false);
		SelectedData->AllocateProperties(PointData->GetAllocatedProperties());

		RemainingData->SetNumPoints(TotalPoints - NumToSelect, /*bInitializeValues=*/false);
		RemainingData->AllocateProperties(PointData->GetAllocatedProperties());

		// ---- 占位数组 ----
		TArray<int32> DummyArray;

		// ---- 异步复制 Selected ----
		FPCGAsync::AsyncProcessing<int32>(
			&Context->AsyncState,
			NumToSelect,
			DummyArray,
			[&](int32 Index, int32& /*OutValue*/) -> int32
			{
				const int32 SrcIndex = SelectedIndices[Index];
				const int32 DstIndex = Index;
				PointData->CopyPropertiesTo(SelectedData, SrcIndex, DstIndex, 1, EPCGPointNativeProperties::All);
				return 0;
			},
			false
		);

		// ---- 异步复制 Remaining ----
		FPCGAsync::AsyncProcessing<int32>(
			&Context->AsyncState,
			TotalPoints - NumToSelect,
			DummyArray,
			[&](int32 Index, int32& /*OutValue*/) -> int32
			{
				const int32 SrcIndex = RemainingIndices[Index];
				const int32 DstIndex = Index;
				PointData->CopyPropertiesTo(RemainingData, SrcIndex, DstIndex, 1, EPCGPointNativeProperties::All);
				return 0;
			},
			false
		);
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
