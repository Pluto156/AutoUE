#include "SLLMEditorWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "HttpModule.h"
#include "Json.h"
#include "JsonUtilities.h"
#include <Elements/PCGSpawnActor.h>

//////////////////////////////////////////////////////////
// UI

void SLLMEditorWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot().FillHeight(0.4f)
				[
					SAssignNew(InputBox, SMultiLineEditableTextBox)
						.HintText(FText::FromString("Enter Prompt..."))
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton)
						.Text(FText::FromString("Send"))
						.OnClicked(this, &SLLMEditorWindow::OnSendClicked)
				]

				+ SVerticalBox::Slot().FillHeight(0.6f)
				[
					SAssignNew(OutputBox, SMultiLineEditableTextBox)
						.IsReadOnly(true)
				]
		];
}

//////////////////////////////////////////////////////////
// 发送按钮

FReply SLLMEditorWindow::OnSendClicked()
{
	// 1️⃣ 获取用户输入
	CachedUserPrompt = InputBox->GetText().ToString();
	if (CachedUserPrompt.IsEmpty())
		return FReply::Handled();

	OutputBox->SetText(FText::FromString(
		"=== Phase1 Input ===\n" + CachedUserPrompt + "\n\n"));

	// 2️⃣ 加载 PCGGraph
	const FString GraphPath = TEXT("/Game/MyPCG/ComposedPCGGraph1.ComposedPCGGraph1");
	UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
	if (!Graph) return FReply::Handled();

	// 3️⃣ 构建模型节点映射
	BuildModelNodeMap(Graph);

	TArray<FString> ModelNames;
	for (auto& Pair : ModelNodeMap)
		ModelNames.Add(Pair.Key);

	// 4️⃣ Phase1 Prompt
	FString Phase1Prompt =
		"Existing models:\n" +
		FString::Join(ModelNames, TEXT(", ")) +
		"\nUser request:\n" +
		CachedUserPrompt +
		"\nReturn JSON: {\"target_models\": [\"ModelName\"]}";

	UE_LOG(LogTemp, Warning, TEXT("=== Phase1 Prompt ===\n%s"), *Phase1Prompt);

	// 5️⃣ Phase1 流式调用
	StartLLMStream(
		Phase1Prompt,
		[](const FString&) {}, // Phase1不显示delta
		[this, GraphPath](const FString& FullContent)
		{
			UE_LOG(LogTemp, Warning, TEXT("=== Phase1 Output ===\n%s"), *FullContent);

			OutputBox->SetText(
				FText::FromString(OutputBox->GetText().ToString() +
					"=== Phase1 Output ===\n" + FullContent + "\n\n"));

			TArray<FString> Targets = ParseTargetModelsFromJSON(FullContent);
			if (Targets.Num() == 0)
			{
				OutputBox->SetText(FText::FromString("No target model found."));
				return;
			}

			// 收集目标节点
			TArray<UPCGNode*> Nodes;
			for (auto& Name : Targets)
				if (ModelNodeMap.Contains(Name))
					Nodes.Append(ModelNodeMap[Name]);

			// 6️⃣ Phase2 Prompt
			FString Phase2Prompt = BuildPromptFromNodes(Nodes, CachedUserPrompt);

			// 明确JSON格式约定，并要求自然语言描述
			Phase2Prompt += TEXT("\n\nFor each node listed above, first provide a brief natural language description of what changes will be made and why, then output a valid JSON array containing the modifications.\n");
			Phase2Prompt += TEXT("JSON array format:\n[\n");
			for (UPCGNode* Node : Nodes)
			{
				if (!Node || !Node->GetSettings()) continue;
				FString NodeType = Node->GetSettings()->GetClass()->GetName();
				Phase2Prompt += FString::Printf(
					TEXT("  {\"type\": \"%s\", \"parameters\": { /* LLM should fill */ }},\n"),
					*NodeType);
			}
			Phase2Prompt += TEXT("]\n");
			Phase2Prompt += TEXT("Only output valid JSON for the array at the end, after the natural language description.\n");

			UE_LOG(LogTemp, Warning, TEXT("=== Phase2 Prompt ===\n%s"), *Phase2Prompt);

			OutputBox->SetText(
				FText::FromString(OutputBox->GetText().ToString() +
					"=== Phase2 Input ===\n" + Phase2Prompt + "\n\n"));

			// 7️⃣ Phase2 流式调用
			StartLLMStream(
				Phase2Prompt,
				[this](const FString& Delta)
				{
					// 显示增量输出（包括自然语言描述和JSON）
					FString Current = OutputBox->GetText().ToString();
					Current += Delta;
					OutputBox->SetText(FText::FromString(Current));
				},
				[this, GraphPath](const FString& FullContent)
				{
					UE_LOG(LogTemp, Warning, TEXT("=== Phase2 Output ===\n%s"), *FullContent);

					OutputBox->SetText(
						FText::FromString(OutputBox->GetText().ToString() +
							"\n\n=== Phase2 Final Output ===\n" + FullContent));

					// 提取 JSON 部分（简单截取最后一个 '[' 开始到结束）用于 ApplyJSONToPCGGraph
					int32 JsonStart = FullContent.Find(TEXT("["), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					FString JsonContent = (JsonStart != INDEX_NONE) ? FullContent.Mid(JsonStart) : FullContent;

					// 写入 JSON 文件
					FString JsonPath = FPaths::ProjectSavedDir() + TEXT("llm_output.json");
					FFileHelper::SaveStringToFile(JsonContent, *JsonPath);

					// 调用 ApplyJSONToPCGGraph 修改 PCGGraph
					ApplyJSONToPCGGraph(JsonPath, GraphPath);
				}
			);
		}
	);

	return FReply::Handled();
}
//////////////////////////////////////////////////////////
// 统一流式函数

void SLLMEditorWindow::StartLLMStream(
	const FString& Prompt,
	TFunction<void(const FString&)> OnDelta,
	TFunction<void(const FString&)> OnCompleted)
{
	int32 RequestId = ++RequestCounter;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetURL("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions");
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetHeader("Authorization", "Bearer YOUR_API_KEY_HERE");

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField("model", "qwen3.5-plus");
	Root->SetBoolField("stream", true);

	TArray<TSharedPtr<FJsonValue>> Messages;
	TSharedPtr<FJsonObject> Msg = MakeShared<FJsonObject>();
	Msg->SetStringField("role", "user");
	Msg->SetStringField("content", Prompt);
	Messages.Add(MakeShared<FJsonValueObject>(Msg));
	Root->SetArrayField("messages", Messages);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	Request->SetContentAsString(Body);

	StreamBufferMap.Add(RequestId, "");
	StreamOffsetMap.Add(RequestId, 0);
	CompletionCallbacks.Add(RequestId, OnCompleted);
	DeltaCallbacks.Add(RequestId, OnDelta);

	Request->OnRequestProgress64().BindLambda(
		[this, RequestId](FHttpRequestPtr Req, uint64, uint64)
		{
			HandleStreamProgress(Req, RequestId);
		});

	Request->OnProcessRequestComplete().BindLambda(
		[this, RequestId](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			HandleStreamCompleted(Req, Resp, bSuccess, RequestId);
		});

	Request->ProcessRequest();
}

//////////////////////////////////////////////////////////
// 流式解析

void SLLMEditorWindow::HandleStreamProgress(FHttpRequestPtr Request, int32 RequestId)
{
	FHttpResponsePtr Response = Request->GetResponse();
	if (!Response.IsValid()) return;

	FString& Buffer = StreamBufferMap[RequestId];
	int32& Offset = StreamOffsetMap[RequestId];

	FString Full = Response->GetContentAsString();
	if (Full.Len() <= Offset) return;

	FString NewChunk = Full.Mid(Offset);
	Offset = Full.Len();

	TArray<FString> Lines;
	NewChunk.ParseIntoArrayLines(Lines);

	for (FString& Line : Lines)
	{
		if (!Line.StartsWith("data: ")) continue;
		FString JsonPart = Line.RightChop(6);
		if (JsonPart == "[DONE]") continue;

		TSharedPtr<FJsonObject> Json;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonPart);
		if (!FJsonSerializer::Deserialize(Reader, Json)) continue;

		const TArray<TSharedPtr<FJsonValue>>* Choices;
		if (Json->TryGetArrayField("choices", Choices))
		{
			auto Obj = (*Choices)[0]->AsObject();
			auto Delta = Obj->GetObjectField("delta");

			if (Delta->HasField("content"))
			{
				FString Text = Delta->GetStringField("content");
				Buffer += Text;

				if (DeltaCallbacks.Contains(RequestId))
					DeltaCallbacks[RequestId](Text);
			}
		}
	}
}

//////////////////////////////////////////////////////////
// 完成

void SLLMEditorWindow::HandleStreamCompleted(
	FHttpRequestPtr,
	FHttpResponsePtr,
	bool,
	int32 RequestId)
{
	if (CompletionCallbacks.Contains(RequestId))
	{
		CompletionCallbacks[RequestId](StreamBufferMap[RequestId]);
	}

	StreamBufferMap.Remove(RequestId);
	StreamOffsetMap.Remove(RequestId);
	CompletionCallbacks.Remove(RequestId);
	DeltaCallbacks.Remove(RequestId);
}

//////////////////////////////////////////////////////////
// 工具函数

void SLLMEditorWindow::BuildModelNodeMap(UPCGGraph* PCGGraph)
{
	ModelNodeMap.Empty();

	if (!PCGGraph) return;

	// 1️⃣ 按行（PositionY）分组
	TMap<float, TArray<UPCGNode*>> NodesByRow;

	for (UPCGNode* Node : PCGGraph->GetNodes())
	{
		if (!Node) continue;

		float Y = Node->PositionY;
		NodesByRow.FindOrAdd(Y).Add(Node);
	}

	// 2️⃣ 对 Y 排序（保持和 ParsePCGGraph 一致）
	TArray<float> SortedY;
	NodesByRow.GetKeys(SortedY);
	SortedY.Sort();

	// 3️⃣ 遍历每一行
	for (float Y : SortedY)
	{
		const TArray<UPCGNode*>& RowNodes = NodesByRow[Y];
		if (RowNodes.Num() == 0) continue;

		FString ModelName = TEXT("UnknownModel");

		// 4️⃣ 找该行最后一个 SpawnActor 节点（和 ParsePCGGraph 一致）
		for (int32 i = RowNodes.Num() - 1; i >= 0; --i)
		{
			UPCGNode* Node = RowNodes[i];
			if (!Node) continue;

			if (UPCGSpawnActorSettings* SpawnSettings =
				Cast<UPCGSpawnActorSettings>(Node->GetSettings()))
			{
				if (SpawnSettings->GetTemplateActorClass())
				{
					ModelName =
						SpawnSettings->GetTemplateActorClass()->GetName();
				}
				break;
			}
		}

		// 5️⃣ 存入 map（整行都归属于这个模型）
		ModelNodeMap.FindOrAdd(ModelName).Append(RowNodes);
	}
}

TArray<FString> SLLMEditorWindow::ParseTargetModelsFromJSON(const FString& Content)
{
	TArray<FString> Result;

	TSharedPtr<FJsonObject> Obj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
	if (!FJsonSerializer::Deserialize(Reader, Obj)) return Result;

	const TArray<TSharedPtr<FJsonValue>>* Array;
	if (Obj->TryGetArrayField("target_models", Array))
	{
		for (auto& Val : *Array)
			Result.Add(Val->AsString());
	}

	return Result;
}

FString SLLMEditorWindow::BuildPromptFromNodes(
	const TArray<UPCGNode*>& TargetNodes,
	const FString& UserPrompt)
{
	FString Prompt;

	const FString ScriptPath =
		TEXT("E:/sourcecode/UELLM/embed/embed_pcg_docs.py");

	const FString CondaPython =
		TEXT("E:/Anaconda/envs/langchainlearn/python.exe");

	const FString CondaEnvPath =
		TEXT("E:/Anaconda/envs/langchainlearn");

	if (!FPaths::FileExists(ScriptPath))
	{
		UE_LOG(LogTemp, Error,
			TEXT("Python script not found: %s"),
			*ScriptPath);

		return TEXT("Error: Python script not found.\n");
	}

	if (!FPaths::FileExists(CondaPython))
	{
		UE_LOG(LogTemp, Error,
			TEXT("Python exe not found: %s"),
			*CondaPython);

		return TEXT("Error: Python executable not found.\n");
	}

	for (UPCGNode* Node : TargetNodes)
	{
		if (!Node || !Node->GetSettings())
			continue;

		FString NodeName = Node->GetSettings()->GetClass()->GetName();

		UE_LOG(LogTemp, Log,
			TEXT("Querying Python docs for node: %s"),
			*NodeName);

		// -----------------------------
		// Query Documentation
		// -----------------------------
		FString DocCommand = FString::Printf(
			TEXT("set PYTHONIOENCODING=utf-8 && ")
			TEXT("set PATH=%s;%%PATH%% && ")
			TEXT("\"%s\" \"%s\" query_documentation \"%s\""),
			*CondaEnvPath,
			*CondaPython,
			*ScriptPath,
			*NodeName
		);

		FString DocArgs = FString::Printf(TEXT("/C \"%s\""), *DocCommand);

		FString DocStdOut;
		FString DocStdErr;
		int32 DocReturnCode = -1;

		FPlatformProcess::ExecProcess(
			TEXT("cmd.exe"),
			*DocArgs,
			&DocReturnCode,
			&DocStdOut,
			&DocStdErr
		);

		// 打印 Python 返回内容
		if (!DocStdOut.IsEmpty())
		{
			UE_LOG(LogTemp, Log,
				TEXT("Python Documentation Output for %s:\n%s"),
				*NodeName,
				*DocStdOut);
		}

		if (!DocStdErr.IsEmpty())
		{
			UE_LOG(LogTemp, Log,
				TEXT("Python Documentation Error for %s:\n%s"),
				*NodeName,
				*DocStdErr);
		}

		// -----------------------------
		// Query Parameter Space
		// -----------------------------
		FString ParamCommand = FString::Printf(
			TEXT("set PYTHONIOENCODING=utf-8 && ")
			TEXT("set PATH=%s;%%PATH%% && ")
			TEXT("\"%s\" \"%s\" query_parameter_space \"%s\""),
			*CondaEnvPath,
			*CondaPython,
			*ScriptPath,
			*NodeName
		);

		FString ParamArgs = FString::Printf(TEXT("/C \"%s\""), *ParamCommand);

		FString ParamStdOut;
		FString ParamStdErr;
		int32 ParamReturnCode = -1;

		FPlatformProcess::ExecProcess(
			TEXT("cmd.exe"),
			*ParamArgs,
			&ParamReturnCode,
			&ParamStdOut,
			&ParamStdErr
		);

		// 打印 Python 返回内容
		if (!ParamStdOut.IsEmpty())
		{
			UE_LOG(LogTemp, Log,
				TEXT("Python Parameter Output for %s:\n%s"),
				*NodeName,
				*ParamStdOut);
		}

		if (!ParamStdErr.IsEmpty())
		{
			UE_LOG(LogTemp, Log,
				TEXT("Python Parameter Error for %s:\n%s"),
				*NodeName,
				*ParamStdErr);
		}

		// -----------------------------
		// Assemble Node Prompt
		// -----------------------------
		Prompt += TEXT("\n==============================\n");
		Prompt += TEXT("Node: ") + NodeName + TEXT("\n");

		if (DocReturnCode == 0)
		{
			Prompt += TEXT("Documentation:\n");
			Prompt += DocStdOut;
			Prompt += TEXT("\n");
		}
		else
		{
			Prompt += TEXT("[Python Documentation Error]\n");
			Prompt += DocStdErr;
			Prompt += TEXT("\n");
		}

		if (ParamReturnCode == 0)
		{
			Prompt += TEXT("Parameter Space:\n");
			Prompt += ParamStdOut;
			Prompt += TEXT("\n");
		}
		else
		{
			Prompt += TEXT("[Python Parameter Space Error]\n");
			Prompt += ParamStdErr;
			Prompt += TEXT("\n");
		}
	}

	Prompt += TEXT("\n==============================\n");
	Prompt += TEXT("User request:\n");
	Prompt += UserPrompt;
	Prompt += TEXT("\n");

	return Prompt;
}
void SLLMEditorWindow::ApplyJSONToPCGGraph(const FString& JsonFilePath, const FString& GraphAssetPath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法读取 JSON 文件: %s"), *JsonFilePath);
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ JSON 解析失败或为空"));
		return;
	}

	UPCGGraph* PCGGraph = LoadObject<UPCGGraph>(nullptr, *GraphAssetPath);
	if (!PCGGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法加载 PCGGraph: %s"), *GraphAssetPath);
		return;
	}

	const FString BlueprintFolder = TEXT("/Game/MyPCG/InteractiveObjectBluePrints");

	// 遍历模型
	for (const TSharedPtr<FJsonValue>& ModelVal : JsonArray)
	{
		TSharedPtr<FJsonObject> ModelObj = ModelVal->AsObject();
		if (!ModelObj.IsValid()) continue;

		FString ModelName;
		if (!ModelObj->TryGetStringField(TEXT("model"), ModelName)) continue;

		const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
		if (!ModelObj->TryGetArrayField(TEXT("nodes"), NodesArray)) continue;

		// 遍历节点
		for (int32 NodeIdx = 0; NodeIdx < NodesArray->Num(); ++NodeIdx)
		{
			TSharedPtr<FJsonObject> NodeObj = (*NodesArray)[NodeIdx]->AsObject();
			if (!NodeObj.IsValid()) continue;

			FString NodeType;
			if (!NodeObj->TryGetStringField(TEXT("type"), NodeType)) continue;

			TSharedPtr<FJsonObject> Params = nullptr;
			if (NodeObj->HasField(TEXT("parameters")))
			{
				Params = NodeObj->GetObjectField(TEXT("parameters"));
			}

			// 找对应节点
			UPCGNode* TargetNode = nullptr;
			for (UPCGNode* Node : PCGGraph->GetNodes())
			{
				if (!Node || !Node->GetSettings()) continue;
				FString NodeSettingsName = Node->GetSettings()->GetClass()->GetName();
				if (NodeSettingsName.Contains(NodeType))
				{
					if (UPCGSpawnActorSettings* SpawnSettings = Cast<UPCGSpawnActorSettings>(Node->GetSettings()))
					{
						FString ExistingModelName;
						if (SpawnSettings->GetTemplateActorClass())
							ExistingModelName = SpawnSettings->GetTemplateActorClass()->GetName();

						if (ExistingModelName.Contains(ModelName))
						{
							TargetNode = Node;
							break;
						}
					}
					else
					{
						TargetNode = Node;
						break;
					}
				}
			}

			if (!TargetNode) continue;
			UPCGSettings* Settings = TargetNode->GetSettings();

			if (Params.IsValid())
			{
				for (const auto& ParamPair : Params->Values)
				{
					FString ParamKey = ParamPair.Key;
					TSharedPtr<FJsonValue> JsonVal = ParamPair.Value;

					FProperty* Property = Settings->GetClass()->FindPropertyByName(FName(*ParamKey));
					if (!Property)
					{
						UE_LOG(LogTemp, Warning, TEXT("⚠️ 属性未找到: %s"), *ParamKey);
						continue;
					}

					// 特殊处理 PCGSpawnActorSettings TemplateActor
					if (Settings->IsA(UPCGSpawnActorSettings::StaticClass()) && ParamKey.Equals(TEXT("TemplateActor"), ESearchCase::IgnoreCase))
					{
						FString TemplateActorStr = JsonVal->AsString();
						if (TemplateActorStr.IsEmpty()) continue;

						FString PureName = TemplateActorStr.StartsWith("A") ? TemplateActorStr.RightChop(1) : TemplateActorStr;
						FString BPBaseName = FString::Printf(TEXT("BP_%s"), *PureName);
						FString BPRuntimePath = FString::Printf(TEXT("%s/%s.%s_C"), *BlueprintFolder, *BPBaseName, *BPBaseName);
						FString BPAssetPath = FString::Printf(TEXT("%s/%s.%s"), *BlueprintFolder, *BPBaseName, *BPBaseName);

						UClass* BPClass = LoadObject<UClass>(nullptr, *BPRuntimePath);
						if (!BPClass)
						{
							UBlueprint* BPAsset = LoadObject<UBlueprint>(nullptr, *BPAssetPath);
							if (BPAsset && BPAsset->GeneratedClass)
								BPClass = BPAsset->GeneratedClass;
						}

						if (!BPClass || !BPClass->IsChildOf(AActor::StaticClass())) continue;

						FProperty* TemplateClassProp = Settings->GetClass()->FindPropertyByName(FName(TEXT("TemplateActorClass")));
						if (FClassProperty* ClassProp = CastField<FClassProperty>(TemplateClassProp))
							ClassProp->SetPropertyValue_InContainer(Settings, BPClass);
					}
				}
			}
		}
	}
}