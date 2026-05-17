// Copyright Epic Games, Inc. All Rights Reserved.
#pragma message(">>> Compiling FLLMHttpServerModule.cpp <<<")

#include "LLMEditor.h"

#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "HttpServerRequest.h"
#include "HttpRouteHandle.h"
#include "IHttpRouter.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Misc/ScopeLock.h"



#include "PCGGraph.h"
#include "Elements/PCGSurfaceSampler.h"
#include "Elements/PCGTypedGetter.h"
#include "AssetToolsModule.h"
#include "Factories/Factory.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"


#if WITH_EDITOR
#include "Editor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#endif
#include <Elements/PCGStaticMeshSpawner.h>
#include <MeshSelectors/PCGMeshSelectorWeighted.h>
#include <Elements/PCGExecuteBlueprint.h>

#define LOCTEXT_NAMESPACE "FLLMHttpServerModule"

void FLLMEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("[LLMHttpServer] Module started1."));
	// ✅ 使用 RegisterStartupCallback 确保菜单系统初始化后再注册
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLLMEditorModule::RegisterMenus)
	);

	UE_LOG(LogTemp, Log, TEXT("[LLMHttpServer] Module started2."));
}


void FLLMEditorModule::ShutdownModule()
{
	// 停止服务并解绑路由
	StopHttpServer();
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	UE_LOG(LogTemp, Display, TEXT("LLMHttpServer 模块已卸载"));
}

void FLLMEditorModule::RegisterMenus()
{
	UE_LOG(LogTemp, Log, TEXT("[LLMHttpServer] RegisterMenus() called."));

	FToolMenuOwnerScoped OwnerScoped(this);

	// 1️⃣ 获取主菜单
	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
	if (!MainMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("[LLMHttpServer] Failed to get LevelEditor.MainMenu."));
		return;
	}

	// 2️⃣ 在主菜单下创建一个新的顶级菜单项 “LLM Tools”
	// 注意：AddSubMenu() 是成员函数，签名与你贴出的一致。
	UToolMenu* LLMMenu = MainMenu->AddSubMenu(
		this,                                           // FToolMenuOwner
		NAME_None,                                      // SectionName（顶级菜单可以不用放在 Section 中）
		"LLMTools",                                     // InName
		LOCTEXT("LLMToolsLabel", "LLM Tools"),          // InLabel
		LOCTEXT("LLMToolsTooltip", "Tools for LLM HTTP Server integration") // InToolTip
	);

	if (!LLMMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("[LLMHttpServer] Failed to create LLM Tools submenu."));
		return;
	}

	// 3️⃣ 在新的 LLM Tools 菜单下添加一个 Section
	FToolMenuSection& Section = LLMMenu->AddSection("LLMHTTP_Main", LOCTEXT("LLMHTTPHeading", "LLM HTTP"));

	// 4️⃣ 添加菜单项
	Section.AddMenuEntry(
		"StartHttpServer",
		LOCTEXT("StartHttpServer", "Start LLM HTTP Server"),
		LOCTEXT("StartHttpServerTooltip", "启动 HTTP Server，用于接收来自 LLM 的消息"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMEditorModule::StartHttpServer))
	);

	Section.AddMenuEntry(
		"StopHttpServer",
		LOCTEXT("StopHttpServer", "Stop LLM HTTP Server"),
		LOCTEXT("StopHttpServerTooltip", "停止 HTTP Server"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMEditorModule::StopHttpServer))
	);

	Section.AddMenuEntry(
		"CreatePCGGraph",
		LOCTEXT("CreatePCGGraph", "Create PCG Graph"),
		LOCTEXT("CreatePCGGraphTooltip", "创建简单的 PCG Graph"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMEditorModule::CreateSimplePCGGraph))
	);
	UE_LOG(LogTemp, Log, TEXT("[LLMHttpServer] ✅ Successfully registered LLM Tools top-level menu."));
}




void FLLMEditorModule::StartHttpServer()
{
	if (bServerStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ HTTP Server 已经在运行"));
		return;
	}

	FHttpServerModule& HttpServerModule = FHttpServerModule::Get();

	// 获取端口对应的路由器（如果不存在会创建）
	HttpRouter = HttpServerModule.GetHttpRouter(ServerPort);
	if (!HttpRouter.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("无法获取 HttpRouter（端口: %d）"), ServerPort);
		return;
	}

	// 绑定路由：/command 接收 POST 请求
	RouteHandle = HttpRouter->BindRoute(
		FHttpPath(TEXT("/command")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda(
			[](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
			{
				const uint8* BodyData = Request.Body.GetData();
				int32 BodyLen = Request.Body.Num();
				FString Body;

				if (BodyData && BodyLen > 0)
				{
					Body = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(BodyData)));
				}

				UE_LOG(LogTemp, Display, TEXT("✅ 收到来自 LLM 的 HTTP 请求:\n%s"), *Body);

				// 回复 OK
				TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(TEXT("OK"), TEXT("text/plain"));
				OnComplete(MoveTemp(Response));
				return true;
			})
	);

	// 启动监听（如果尚未启动）
	HttpServerModule.StartAllListeners();

	bServerStarted = true;
	UE_LOG(LogTemp, Display, TEXT("🚀 LLM HTTP Server 已启动: http://127.0.0.1:%d/command"), ServerPort);
}

void FLLMEditorModule::StopHttpServer()
{
	if (!bServerStarted)
	{
		return;
	}

	// 解绑路由
	if (HttpRouter.IsValid() && RouteHandle.IsValid())
	{
		HttpRouter->UnbindRoute(RouteHandle);
		RouteHandle = FHttpRouteHandle(); // reset
	}

	// 停止监听所有监听器（如果需要只停止特定端口，可改为更细粒度控制）
	FHttpServerModule::Get().StopAllListeners();

	HttpRouter.Reset();
	bServerStarted = false;

	UE_LOG(LogTemp, Display, TEXT("🛑 LLM HTTP Server 已停止"));
}


void FLLMEditorModule::CreateSimplePCGGraph()
{
	const FString PackageName = TEXT("/Game/PCG/MyGeneratedGraph");
	const FString AssetName = TEXT("MyGeneratedGraph");

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad(); // ✅ 确保包是 Fully Loaded

	UPCGGraph* PCGGraph = NewObject<UPCGGraph>(Package, UPCGGraph::StaticClass(), *AssetName, RF_Public | RF_Standalone);
	if (!PCGGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法创建 PCGGraph"));
		return;
	}

	// ==========================================================
	// ✅ Step 1. 遍历所有 PCG 节点类并提取引脚信息
	// ==========================================================
	FString OutputText;
	OutputText += TEXT("=== PCG Node Pin List ===\n\n");

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (Class->IsChildOf(UPCGSettings::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
		{
			UPCGSettings* Settings = NewObject<UPCGSettings>(GetTransientPackage(), Class);

			if (!Settings)
			{
				continue;
			}

			TArray<FPCGPinProperties> InputPins = Settings->InputPinProperties();
			TArray<FPCGPinProperties> OutputPins = Settings->OutputPinProperties();

			OutputText += FString::Printf(TEXT("[%s]\n"), *Class->GetName());

			if (InputPins.Num() > 0)
			{
				OutputText += TEXT("  Input Pins:\n");
				for (const FPCGPinProperties& Pin : InputPins)
				{
					OutputText += FString::Printf(TEXT("    - %s (%s)\n"),
						*Pin.Label.ToString(),
						*UEnum::GetValueAsString(Pin.AllowedTypes));
				}
			}
			else
			{
				OutputText += TEXT("  Input Pins: (None)\n");
			}

			if (OutputPins.Num() > 0)
			{
				OutputText += TEXT("  Output Pins:\n");
				for (const FPCGPinProperties& Pin : OutputPins)
				{
					OutputText += FString::Printf(TEXT("    - %s (%s)\n"),
						*Pin.Label.ToString(),
						*UEnum::GetValueAsString(Pin.AllowedTypes));
				}
			}
			else
			{
				OutputText += TEXT("  Output Pins: (None)\n");
			}

			OutputText += TEXT("\n");
		}
	}

	// ✅ 写入文件
	const FString FilePath = FPaths::ProjectSavedDir() / TEXT("PCG_PinList.txt");
	if (FFileHelper::SaveStringToFile(OutputText, *FilePath))
	{
		UE_LOG(LogTemp, Display, TEXT("✅ 所有PCG节点引脚信息已写入: %s"), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法写入引脚信息文件: %s"), *FilePath);
	}

	

	// ==========================================================
	// ✅ Step 2. 创建示例 PCG Graph
	// ==========================================================
	UPCGGetLandscapeSettings* GetLandscapeSettings = NewObject<UPCGGetLandscapeSettings>(PCGGraph);
	UPCGSurfaceSamplerSettings* SurfaceSamplerSettings = NewObject<UPCGSurfaceSamplerSettings>(PCGGraph);


	// ✅ 创建蓝图节点（假设你的蓝图类路径为 /Game/PCG/Nodes/BP_SelectRandomPoint.BP_SelectRandomPoint）
	UClass* SelectRandomPointClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/MyPCG/CustomPCGNode/SelectRandomPoint.SelectRandomPoint_C")
	);

	UPCGBlueprintSettings* SelectRandomPointSettings = nullptr;
	if (SelectRandomPointClass)
	{
		SelectRandomPointSettings = NewObject<UPCGBlueprintSettings>(PCGGraph, SelectRandomPointClass);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法加载蓝图类：BP_SelectRandomPoint"));
	}


	UPCGNode* GetLandscapeNode = PCGGraph->AddNode(GetLandscapeSettings);
	UPCGNode* SurfaceSamplerNode = PCGGraph->AddNode(SurfaceSamplerSettings);
	UPCGNode* SelectRandomPointNode = nullptr;

	if (SelectRandomPointSettings)
	{
		SelectRandomPointNode = PCGGraph->AddNode(SelectRandomPointSettings);
	}

	SurfaceSamplerSettings->PointsPerSquaredMeter = 0.5f;
	SurfaceSamplerSettings->PointExtents = FVector(50.0f, 50.0f, 100.0f);
	SurfaceSamplerSettings->Looseness = 0.8f;

	GetLandscapeNode->SetNodePosition(0.f, 0.f);
	SurfaceSamplerNode->SetNodePosition(400.f, 0.f);

	const FName OutputPin = TEXT("Out");
	const FName InputPin = TEXT("Surface");
	const FName InputPin_Random = TEXT("In");
	const FName OutputPin_Random = TEXT("Out");
	if (UPCGPin* OutPin = GetLandscapeNode->GetOutputPin(OutputPin))
	{
		if (UPCGPin* InPin = SurfaceSamplerNode->GetInputPin(InputPin))
		{
			OutPin->AddEdgeTo(InPin);
		}
	}

	if (SelectRandomPointNode)
	{
		if (UPCGPin* OutPin2 = SurfaceSamplerNode->GetOutputPin(OutputPin))
		{
			if (UPCGPin* InPin2 = SelectRandomPointNode->GetInputPin(InputPin_Random))
			{
				OutPin2->AddEdgeTo(InPin2);
			}
		}
	}

	// ✅ 注册资产
	FAssetRegistryModule::AssetCreated(PCGGraph);
	PCGGraph->MarkPackageDirty();

	// ✅ 保存资产
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.Error = GWarn;
	SaveArgs.bForceByteSwapping = false;

	if (UPackage::SavePackage(Package, PCGGraph, *PackageFileName, SaveArgs))
	{
		UE_LOG(LogTemp, Display, TEXT("✅ PCG Graph 已成功保存: %s"), *PackageFileName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 保存 PCG Graph 失败"));
	}
}


void FLLMEditorModule::CreatePCGGraphFromJSON()
{
	CreatePCGGraphFromJSON(FPaths::ProjectSavedDir() / TEXT("PCG/HouseWithFence_PCGGraph.json"));
}

void FLLMEditorModule::CreatePCGGraphFromJSON(const FString& JsonFilePath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法读取 JSON 文件: %s"), *JsonFilePath);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ JSON 解析失败"));
		return;
	}

	const FString PackageName = TEXT("/Game/PCG/") + JsonObject->GetStringField("graph_name");
	const FString AssetName = JsonObject->GetStringField("graph_name");

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	UPCGGraph* PCGGraph = NewObject<UPCGGraph>(Package, UPCGGraph::StaticClass(), *AssetName, RF_Public | RF_Standalone);
	if (!PCGGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法创建 PCGGraph"));
		return;
	}

	// ==========================================================
	// Step 1. 创建节点
	// ==========================================================
	TMap<FString, UPCGNode*> NodeMap;

	const TArray<TSharedPtr<FJsonValue>>* JsonNodes;
	if (JsonObject->TryGetArrayField("nodes", JsonNodes))
	{
		for (const TSharedPtr<FJsonValue>& NodeVal : *JsonNodes)
		{
			TSharedPtr<FJsonObject> NodeObj = NodeVal->AsObject();
			if (!NodeObj.IsValid()) continue;

			FString NodeId = NodeObj->GetStringField("id");
			FString NodeType = NodeObj->GetStringField("type");
			TSharedPtr<FJsonObject> Params = NodeObj->GetObjectField("parameters");

			// 查找对应的 Settings 类
			UClass* SettingsClass = nullptr;
			for (TObjectIterator<UClass> It; It; ++It)
			{
				UClass* Class = *It;
				if (Class->IsChildOf(UPCGSettings::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
				{
					if (Class->GetName().Contains(NodeType))
					{
						SettingsClass = Class;
						break;
					}
				}
			}

			if (!SettingsClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 找不到对应的 PCGSettings 类: %s"), *NodeType);
				continue;
			}

			UPCGSettings* Settings = NewObject<UPCGSettings>(PCGGraph, SettingsClass);
			UPCGNode* Node = PCGGraph->AddNode(Settings);

			// ==========================================================
			// ✅ Step 1.1 设置节点位置
			// ==========================================================
			if (NodeObj->HasField("position"))
			{
				const TArray<TSharedPtr<FJsonValue>>* PosArray;
				if (NodeObj->TryGetArrayField("position", PosArray) && PosArray->Num() >= 2)
				{
					float X = static_cast<float>((*PosArray)[0]->AsNumber());
					float Y = static_cast<float>((*PosArray)[1]->AsNumber());
					Node->SetNodePosition(X, Y);
					UE_LOG(LogTemp, Display, TEXT("📍 节点位置: %s -> (%.1f, %.1f)"), *NodeId, X, Y);
				}
			}

			// ==========================================================
			// Step 1.2 （可选）设置节点参数（使用反射）
			// ==========================================================
			for (const auto& ParamPair : Params->Values)
			{
				const FString& ParamKey = ParamPair.Key;
				const FName ParamName(*ParamKey);

				FProperty* Property = Settings->GetClass()->FindPropertyByName(ParamName);
				if (!Property)
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠️ 找不到属性: %s 在 %s 中"), *ParamKey, *Settings->GetClass()->GetName());
					continue;
				}

				const TSharedPtr<FJsonValue>& JsonVal = ParamPair.Value;
				void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Settings);

				// -------------------- 数值类型 --------------------
				if (FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
				{
					if (JsonVal->Type == EJson::Number)
					{
						double Num = JsonVal->AsNumber();
						if (NumProp->IsInteger())
						{
							NumProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(Num));
						}
						else
						{
							NumProp->SetFloatingPointPropertyValue(ValuePtr, Num);
						}
					}
					else if (JsonVal->Type == EJson::Boolean)
					{
						bool bVal = JsonVal->AsBool();
						if (NumProp->IsInteger())
							NumProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(bVal ? 1 : 0));
						else
							NumProp->SetFloatingPointPropertyValue(ValuePtr, bVal ? 1.0 : 0.0);
					}
					else if (JsonVal->Type == EJson::String)
					{
						// 尝试将字符串解析为数值（如 "1.0", "42"）
						FString StrVal = JsonVal->AsString();
						if (NumProp->IsInteger())
						{
							int64 IntVal = FCString::Atoi(*StrVal);
							NumProp->SetIntPropertyValue(ValuePtr, IntVal);
						}
						else
						{
							double FloatVal = FCString::Atod(*StrVal);
							NumProp->SetFloatingPointPropertyValue(ValuePtr, FloatVal);
						}
					}
				}

				// -------------------- 布尔类型 --------------------
				else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
				{
					BoolProp->SetPropertyValue(ValuePtr, JsonVal->AsBool());
				}

				// -------------------- 字符串类型 --------------------
				else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
				{
					StrProp->SetPropertyValue(ValuePtr, JsonVal->AsString());
				}
				else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
				{
					NameProp->SetPropertyValue(ValuePtr, FName(*JsonVal->AsString()));
				}
				// ------------------------------
// FVector
// ------------------------------
				else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
				{
					if (StructProp->Struct == TBaseStructure<FVector>::Get())
					{
						FVector* Vec = StructProp->ContainerPtrToValuePtr<FVector>(Settings);
						if (!Vec)
						{
						}
						// { "X": 1, "Y": 2, "Z": 3 }
						else if (JsonVal->Type == EJson::Object)
						{
							const TSharedPtr<FJsonObject> Obj = JsonVal->AsObject();
							if (Obj.IsValid() &&
								Obj->HasTypedField<EJson::Number>(TEXT("X")) &&
								Obj->HasTypedField<EJson::Number>(TEXT("Y")) &&
								Obj->HasTypedField<EJson::Number>(TEXT("Z")))
							{
								Vec->X = Obj->GetNumberField(TEXT("X"));
								Vec->Y = Obj->GetNumberField(TEXT("Y"));
								Vec->Z = Obj->GetNumberField(TEXT("Z"));
							}
						}
						// [1, 2, 3]
						else if (JsonVal->Type == EJson::Array)
						{
							const TArray<TSharedPtr<FJsonValue>>& Arr = JsonVal->AsArray();
							if (Arr.Num() == 3)
							{
								Vec->X = Arr[0]->AsNumber();
								Vec->Y = Arr[1]->AsNumber();
								Vec->Z = Arr[2]->AsNumber();
							}
						}
					}
				}
				// -------------------- 数组类型 --------------------
				else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
				{
					const TArray<TSharedPtr<FJsonValue>>* ArrayVals;
					if (JsonVal->TryGetArray(ArrayVals))
					{
						FScriptArrayHelper Helper(ArrayProp, ValuePtr);
						Helper.EmptyValues();

						FProperty* InnerProp = ArrayProp->Inner;
						if (FNumericProperty* InnerNumProp = CastField<FNumericProperty>(InnerProp))
						{
							for (const auto& Elem : *ArrayVals)
							{
								int32 Index = Helper.AddValue();
								void* Dest = Helper.GetRawPtr(Index);
								double Val = Elem->AsNumber();

								if (InnerNumProp->IsInteger())
									InnerNumProp->SetIntPropertyValue(Dest, static_cast<int64>(Val));
								else
									InnerNumProp->SetFloatingPointPropertyValue(Dest, Val);
							}
						}
						else if (FStrProperty* InnerStrProp = CastField<FStrProperty>(InnerProp))
						{
							for (const auto& Elem : *ArrayVals)
							{
								int32 Index = Helper.AddValue();
								InnerStrProp->SetPropertyValue(Helper.GetRawPtr(Index), Elem->AsString());
							}
						}
					}
				}


				// -------------------- 未支持类型 --------------------
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠️ 暂不支持的属性类型: %s"), *Property->GetClass()->GetName());
				}
			}


			// ==========================================================
			// ✅ 特殊处理：UPCGStaticMeshSpawnerSimpleSettings
			// ==========================================================
			UE_LOG(LogTemp, Warning, TEXT("节点名字："), *(Settings->GetName()));
			if (Settings->IsA<UPCGStaticMeshSpawnerSettings>())
			{
				FString MeshPath;
				FProperty* MeshPathProp = Settings->GetClass()->FindPropertyByName(TEXT("StaticMeshPath"));
				if (MeshPathProp && CastField<FStrProperty>(MeshPathProp))
				{
					MeshPath = CastFieldChecked<FStrProperty>(MeshPathProp)->GetPropertyValue_InContainer(Settings);
				}

				UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
				if (!Mesh)
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法加载静态网格资源: %s，使用默认Cube代替"), *MeshPath);
					Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/DebugObjects/PCG_Cube.PCG_Cube"));

				}

				if (!Mesh)
				{
					UE_LOG(LogTemp, Error, TEXT("❌ 默认Cube也加载失败，请检查引擎内容是否启用！"));
				}
				else
				{
					UE_LOG(LogTemp, Display, TEXT("✅ 已为节点 %s 设置静态网格: %s"), *NodeId, *Mesh->GetName());
				}

				UPCGStaticMeshSpawnerSettings* StaticMeshSpawnerSettings = CastChecked<UPCGStaticMeshSpawnerSettings>(Settings);
				StaticMeshSpawnerSettings->SetMeshSelectorType(UPCGMeshSelectorWeighted::StaticClass());
				UPCGMeshSelectorWeighted* MeshSelector = CastChecked<UPCGMeshSelectorWeighted>(StaticMeshSpawnerSettings->MeshSelectorParameters);
				TArray<FPCGMeshSelectorWeightedEntry> Entries;
				Entries.Emplace(Mesh, 1);
				MeshSelector->MeshEntries = Entries;
			}




			NodeMap.Add(NodeId, Node);
		}
	}

	// ==========================================================
	// Step 2. 创建连接
	// ==========================================================
	const TArray<TSharedPtr<FJsonValue>>* JsonConnections;
	if (JsonObject->TryGetArrayField("connections", JsonConnections))
	{
		for (const TSharedPtr<FJsonValue>& ConnVal : *JsonConnections)
		{
			TSharedPtr<FJsonObject> ConnObj = ConnVal->AsObject();
			if (!ConnObj.IsValid()) continue;

			FString From = ConnObj->GetStringField("from");
			FString To = ConnObj->GetStringField("to");

			FString FromNodeId, FromPinName;
			FString ToNodeId, ToPinName;

			if (From.Split(TEXT("."), &FromNodeId, &FromPinName) && To.Split(TEXT("."), &ToNodeId, &ToPinName))
			{
				UPCGNode** FromNodePtr = NodeMap.Find(FromNodeId);
				UPCGNode** ToNodePtr = NodeMap.Find(ToNodeId);

				if (FromNodePtr && ToNodePtr)
				{
					UPCGNode* FromNode = *FromNodePtr;
					UPCGNode* ToNode = *ToNodePtr;

					if (UPCGPin* OutPin = FromNode->GetOutputPin(FName(*FromPinName)))
					{
						if (UPCGPin* InPin = ToNode->GetInputPin(FName(*ToPinName)))
						{
							OutPin->AddEdgeTo(InPin);
						}
					}
				}
			}
		}
	}

	// ==========================================================
	// Step 3. 注册和保存
	// ==========================================================
	FAssetRegistryModule::AssetCreated(PCGGraph);
	PCGGraph->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.Error = GWarn;
	SaveArgs.bForceByteSwapping = false;

	if (UPackage::SavePackage(Package, PCGGraph, *PackageFileName, SaveArgs))
	{
		UE_LOG(LogTemp, Display, TEXT("✅ PCG Graph 已成功保存: %s"), *PackageFileName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 保存 PCG Graph 失败"));
	}
}



#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLLMEditorModule, LLMEditor)
