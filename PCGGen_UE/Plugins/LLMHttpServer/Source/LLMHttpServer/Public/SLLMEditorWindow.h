#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGSettings.h"

class SLLMEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLLMEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	// UI
	FReply OnSendClicked();

	// ===== 统一流式 LLM 调用 =====
	void StartLLMStream(
		const FString& Prompt,
		TFunction<void(const FString&)> OnDelta,
		TFunction<void(const FString&)> OnCompleted
	);

	void HandleStreamProgress(FHttpRequestPtr Request, int32 RequestId);
	void HandleStreamCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess, int32 RequestId);

	// Graph
	void BuildModelNodeMap(UPCGGraph* PCGGraph);
	FString BuildPromptFromNodes(const TArray<UPCGNode*>& Nodes, const FString& UserPrompt);
	TArray<FString> ParseTargetModelsFromJSON(const FString& Content);
	void ApplyJSONToPCGGraph(const FString& JsonFilePath, const FString& GraphAssetPath);

private:

	TSharedPtr<class SMultiLineEditableTextBox> InputBox;
	TSharedPtr<class SMultiLineEditableTextBox> OutputBox;

	TMap<int32, FString> StreamBufferMap;
	TMap<int32, int32> StreamOffsetMap;
	TMap<int32, TFunction<void(const FString&)>> CompletionCallbacks;
	TMap<int32, TFunction<void(const FString&)>> DeltaCallbacks;

	int32 RequestCounter = 0;

	TMap<FString, TArray<UPCGNode*>> ModelNodeMap;
	FString CachedUserPrompt;
};