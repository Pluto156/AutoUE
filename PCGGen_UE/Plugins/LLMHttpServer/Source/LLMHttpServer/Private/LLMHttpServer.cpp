// Copyright Epic Games, Inc. All Rights Reserved.
#pragma message(">>> Compiling FLLMHttpServerModule.cpp <<<")

#include "LLMHttpServer.h"

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
#include "CustomPCGNode/PCGSelectRandomPoints.h"
#include <Kismet2/KismetEditorUtilities.h>
#include <Elements/PCGSpawnActor.h>
#include <ObjectTools.h>


#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "GenericPlatform/GenericPlatformHttp.h"

#include "Factories/FbxImportUI.h"
#include "Factories/FbxFactory.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "AssetImportTask.h"
#include <Factories/FbxStaticMeshImportData.h>
#include <Factories/FbxTextureImportData.h>
#include <Factories/FbxAnimSequenceImportData.h>
#include <Factories/FbxSkeletalMeshImportData.h>

#include "Gltf/InterchangeGltfTranslator.h"
#include "InterchangeglTFPipeline.h"
#include "InterchangeManager.h"
#include "InterchangeGenericAssetsPipeline.h"
#include "InterchangeGenericMeshPipeline.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"


#include "ThumbnailRendering/ThumbnailManager.h"
#include "ThumbnailRendering/ThumbnailRenderer.h"
#include "ThumbnailRendering/StaticMeshThumbnailRenderer.h"

#include "ImageUtils.h"
#include "Engine/TextureRenderTarget2D.h"



// ================= SceneCapture =================
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"

// ================= 灯光 =================
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"


// ================= 渲染 / Streaming =================
#include "Engine/Texture.h"
#include "Engine/StaticMeshActor.h"
#include "RenderingThread.h"
#include "RenderResource.h"

// ================= Image / PNG =================
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include <Engine/SkyLight.h>

#include "PhysicsEngine/BodySetup.h"

#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFile.h"
#include "Logging/LogMacros.h"

#include "LLMEditor/Public/CustomPCGNode/PCGPromptSpatialDistribution.h"
#include <JsonObjectConverter.h>

#include "SLLMEditorWindow.h"

#define LOCTEXT_NAMESPACE "FLLMHttpServerModule"

void FLLMHttpServerModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("[LLMHttpServer] Module started1."));
	// ✅ 使用 RegisterStartupCallback 确保菜单系统初始化后再注册
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLLMHttpServerModule::RegisterMenus)
	);

	UE_LOG(LogTemp, Log, TEXT("[LLMHttpServer] Module started2."));
}


void FLLMHttpServerModule::ShutdownModule()
{
	// 停止服务并解绑路由
	StopHttpServer();
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	UE_LOG(LogTemp, Display, TEXT("LLMHttpServer 模块已卸载"));
}

void FLLMHttpServerModule::RegisterMenus()
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
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::StartHttpServer))
	);

	Section.AddMenuEntry(
		"StopHttpServer",
		LOCTEXT("StopHttpServer", "Stop LLM HTTP Server"),
		LOCTEXT("StopHttpServerTooltip", "停止 HTTP Server"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::StopHttpServer))
	);

	Section.AddMenuEntry(
		"CreatePCGGraph",
		LOCTEXT("CreatePCGGraph", "Create PCG Graph"),
		LOCTEXT("CreatePCGGraphTooltip", "创建简单的 PCG Graph"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::CreateSimplePCGGraph))
	);

	Section.AddMenuEntry(
		"GenerateBlueprintsFromSpawnActorJSON",
		LOCTEXT("GenerateBlueprintsFromSpawnActorJSON", "Create blueprint From Json"),
		LOCTEXT("GenerateBlueprintsFromSpawnActorJSONTooltip", "通过Json创建可交互物体类"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::GenerateBlueprintsFromSpawnActorJSON))
	);
	Section.AddMenuEntry(
		"CreatePCGGraphNodeFromJson",
		LOCTEXT("CreatePCGGraphNodeFromJson", "Create PCG Graph Node From Json"),
		LOCTEXT("CreatePCGGraphNodeFromJsonTooltip", "通过Json创建PCG Graph 节点"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::CreatePCGGraphNodesFromJSON))
	);
	Section.AddMenuEntry(
		"SearchFabModelsFromJSON",
		LOCTEXT("SearchFabModelsFromJSON", "SearchFabModelsFromJSON"),
		LOCTEXT("SearchFabModelsFromJSONTooltip", "搜索模型"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::SearchFabModelsFromJSON))
	);
	Section.AddMenuEntry(
		"ImportFbx",
		LOCTEXT("ImportFbx", "ImportFbx"),
		LOCTEXT("ImportFbx", "导入fbx模型"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::ImportFbx))
	);
	Section.AddMenuEntry(
		"ImportGLTF",
		LOCTEXT("ImportGLTF", "ImportGLTF"),
		LOCTEXT("ImportGLTF", "导入GLTF模型"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::ImportGLTF))
	);
	Section.AddMenuEntry(
		"TakeModelPicture",
		LOCTEXT("TakeModelPicture", "TakeModelPicture"),
		LOCTEXT("TakeModelPicture", "TakeModelPicture"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::TakeModelPicture))
	);
	Section.AddMenuEntry(
		"SaveDemo",
		LOCTEXT("SaveDemo", "SaveDemo"),
		LOCTEXT("SaveDemo", "SaveDemo"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::SaveDemo))
	);
	Section.AddMenuEntry(
		"GeneratePCGGraphLogFromJSON",
		LOCTEXT("GeneratePCGGraphLogFromJSON", "GeneratePCGGraphLogFromJSON"),
		LOCTEXT("GeneratePCGGraphLogFromJSON", "GeneratePCGGraphLogFromJSON"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::GeneratePCGGraphLogFromJSON))
	);

	Section.AddMenuEntry(
		"GenerateRuleStruct",
		LOCTEXT("GenerateRuleStruct", "GenerateRuleStruct"),
		LOCTEXT("GenerateRuleStruct", "GenerateRuleStruct"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::GenerateRuleStruct))
	);

	// editor窗口

	FLevelEditorModule& LevelEditorModule =
		FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

	MenuExtender->AddMenuBarExtension(
		"Window",
		EExtensionHook::After,
		nullptr,
		FMenuBarExtensionDelegate::CreateLambda([this](FMenuBarBuilder& Builder)
			{
				Builder.AddPullDownMenu(
					LOCTEXT("LLMMenu", "LLM"),
					LOCTEXT("LLMMenuTooltip", "LLM Tools"),
					FNewMenuDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
						{
							MenuBuilder.AddMenuEntry(
								LOCTEXT("OpenLLMWindow", "Open LLM Editor"),
								LOCTEXT("OpenLLMWindowTooltip", "Open LLM Prompt Window"),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateRaw(this, &FLLMHttpServerModule::OpenLLMWindow))
							);
						})
				);
			})
	);

	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);


	UE_LOG(LogTemp, Log, TEXT("[LLMHttpServer] ✅ Successfully registered LLM Tools top-level menu."));
}


void FLLMHttpServerModule::OpenLLMWindow()
{
	if (LLMWindow.IsValid())
	{
		LLMWindow.Pin()->BringToFront();
		return;
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString("LLM Editor"))
		.ClientSize(FVector2D(700, 500))
		.SupportsMaximize(true)
		.SupportsMinimize(true);

	Window->SetContent(
		SNew(SLLMEditorWindow)
	);

	LLMWindow = Window;

	FSlateApplication::Get().AddWindow(Window);
}

void FLLMHttpServerModule::StartHttpServer()
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

void FLLMHttpServerModule::StopHttpServer()
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


void FLLMHttpServerModule::CreateSimplePCGGraph()
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
	//OutputText += TEXT("=== PCG Node Pin List ===\n\n");

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
// ✅ Step 2. 创建示例 PCG Graph（含自定义 SelectRandomPoint 蓝图节点）
// ==========================================================
	UPCGGetLandscapeSettings* GetLandscapeSettings = NewObject<UPCGGetLandscapeSettings>(PCGGraph);
	UPCGSurfaceSamplerSettings* SurfaceSamplerSettings = NewObject<UPCGSurfaceSamplerSettings>(PCGGraph);
	UPCGSelectRandomPointsSettings* SelectRandomPointsSettings = NewObject<UPCGSelectRandomPointsSettings>(PCGGraph);





	// ✅ 添加节点到 Graph
	UPCGNode* GetLandscapeNode = PCGGraph->AddNode(GetLandscapeSettings);
	UPCGNode* SurfaceSamplerNode = PCGGraph->AddNode(SurfaceSamplerSettings);
	UPCGNode* SelectRandomPointNode = PCGGraph->AddNode(SelectRandomPointsSettings);



	// ✅ 参数设置
	SurfaceSamplerSettings->PointsPerSquaredMeter = 0.5f;
	SurfaceSamplerSettings->PointExtents = FVector(50.0f, 50.0f, 100.0f);
	SurfaceSamplerSettings->Looseness = 0.8f;

	// ✅ 节点布局
	GetLandscapeNode->SetNodePosition(0.f, 0.f);
	SurfaceSamplerNode->SetNodePosition(400.f, 0.f);
	if (SelectRandomPointNode)
	{
		SelectRandomPointNode->SetNodePosition(800.f, 0.f);
	}

	// ✅ 连接节点
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

	// ==========================================================
	// ✅ 注册与保存资产
	// ==========================================================
	FAssetRegistryModule::AssetCreated(PCGGraph);
	PCGGraph->MarkPackageDirty();

	const FString PackageFileName =
		FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

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

void FLLMHttpServerModule::CreatePCGGraphNodesFromJSON()
{
	//CreatePCGGraphNodesFromJSON(FPaths::ProjectSavedDir() / TEXT("PCG/Node.json"), FPaths::ProjectSavedDir() / TEXT("PCG/Pin.json"));
	LinkPCGGraphNodesFromJSON(FPaths::ProjectSavedDir() / TEXT("PCG/ComposedGraph.json"));

}

void FLLMHttpServerModule::GenerateBlueprintsFromSpawnActorJSON()
{
	GenerateBlueprintsFromSpawnActorJSON(FPaths::ProjectSavedDir() / TEXT("PCG/ComposedGraph.json"), FPaths::ProjectSavedDir() / TEXT("PCG/RetriveModel.json"), FPaths::ProjectSavedDir() / TEXT("PCG/InteractiveObjectAnalyzer.json"));

}

void FLLMHttpServerModule::SearchFabModelsFromJSON()
{
	SearchFabModelsFromJSON(FPaths::ProjectSavedDir() / TEXT("PCG/Models.json"), TEXT("513813d77d944b4db997b2fb2b97c42d"));
}

void FLLMHttpServerModule::ImportFbx()
{
	FString SourceFolder = TEXT("E:/sourcecode/UELLM/model_description/fbx");
	FString TargetRoot = TEXT("/Game/MyPCG/models");

	FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
	IAssetTools& AssetTools = AssetToolsModule.Get();

	// 收集所有 .fbx
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(
		Files,
		*SourceFolder,
		TEXT("*.fbx"),
		true, false
	);

	UE_LOG(LogTemp, Warning, TEXT("📦 Found %d FBX files."), Files.Num());

	for (const FString& FilePath : Files)
	{
		FString FileName = FPaths::GetBaseFilename(FilePath);
		FString DestPath = TargetRoot + TEXT("/") + FileName;

		UE_LOG(LogTemp, Warning, TEXT("\n=============================="));
		UE_LOG(LogTemp, Warning, TEXT("🎯 Processing FBX: %s"), *FilePath);
		UE_LOG(LogTemp, Warning, TEXT("📁 Destination:  %s"), *DestPath);

		// =====================================================
		//  Step 1：检查是否已经导入过（避免重复导入）
		// =====================================================
		FString ExistingAssetPath = DestPath + TEXT(".") + FileName;
		FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FAssetData FoundAsset = AssetRegistry.Get().GetAssetByObjectPath(*ExistingAssetPath);

		if (FoundAsset.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("⏩ Skip: Asset already exists: %s"), *ExistingAssetPath);
			continue;
		}

		// =====================================================
		//  Step 2：配置 FBX 导入设置
		// =====================================================
		UE_LOG(LogTemp, Warning, TEXT("⚙️  Creating ImportUI..."));

		UFbxImportUI* ImportUI = NewObject<UFbxImportUI>();
		ImportUI->bAutomatedImportShouldDetectType = false;
		ImportUI->OriginalImportType = FBXIT_StaticMesh;
		ImportUI->MeshTypeToImport = FBXIT_StaticMesh;
		ImportUI->bImportAsSkeletal = false;
		ImportUI->bImportRigidMesh = true;

		// -------- 材质与贴图 --------
		ImportUI->bImportMaterials = true;
		ImportUI->bImportTextures = true;

		UE_LOG(LogTemp, Warning, TEXT("🎨 Import Materials: %d"), ImportUI->bImportMaterials);
		UE_LOG(LogTemp, Warning, TEXT("🖼️  Import Textures : %d"), ImportUI->bImportTextures);

		// =====================================================
		// StaticMesh 导入设置
		// =====================================================
		UFbxStaticMeshImportData* StaticMeshData = NewObject<UFbxStaticMeshImportData>();

		StaticMeshData->bCombineMeshes = true;
		StaticMeshData->bGenerateLightmapUVs = true;
		StaticMeshData->bAutoGenerateCollision = true;

		UE_LOG(LogTemp, Warning, TEXT("🧩 Combine Meshes: %d"), StaticMeshData->bCombineMeshes);

		StaticMeshData->VertexColorImportOption = EVertexColorImportOption::Ignore;
		ImportUI->StaticMeshImportData = StaticMeshData;

		// -------- Texture 导入设置 --------
		ImportUI->TextureImportData = NewObject<UFbxTextureImportData>();
		ImportUI->TextureImportData->bUseBaseMaterial = false;

		UE_LOG(LogTemp, Warning, TEXT("🧪 TextureImportData created. BaseMaterial: %d"),
			ImportUI->TextureImportData->bUseBaseMaterial);

		ImportUI->SetMeshTypeToImport();

		// =====================================================
		// Step 3：构建 AssetImportTask
		// =====================================================
		UAssetImportTask* Task = NewObject<UAssetImportTask>();
		Task->Filename = FilePath;
		Task->DestinationPath = DestPath;
		Task->bReplaceExisting = false;
		Task->bAutomated = true;
		Task->bSave = true;
		Task->Options = ImportUI;

		UE_LOG(LogTemp, Warning, TEXT("🚀 Starting import task..."));

		AssetTools.ImportAssetTasks({ Task });

		// =====================================================
		// 调试：打印所有导入结果
		// =====================================================
		if (Task->ImportedObjectPaths.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("❌ No objects were imported! Possible FBX error or empty file."));
		}

		for (const FString& ImportedAssetPath : Task->ImportedObjectPaths)
		{
			UE_LOG(LogTemp, Warning, TEXT("✅ Imported: %s"), *ImportedAssetPath);
		}

		// =====================================================
		// 调试：检查材质是否真的被导入
		// =====================================================
		TArray<FAssetData> AssetsInFolder;
		AssetRegistry.Get().GetAssetsByPath(*DestPath, AssetsInFolder, true);

		bool HasMaterial = false;
		bool HasTexture = false;

		for (const FAssetData& Asset : AssetsInFolder)
		{
			FString ClassName = Asset.AssetClassPath.ToString();

			if (ClassName.Contains("Material"))
			{
				UE_LOG(LogTemp, Warning, TEXT("🎨 Material detected: %s"), *Asset.GetFullName());
				HasMaterial = true;
			}

			if (ClassName.Contains("Texture"))
			{
				UE_LOG(LogTemp, Warning, TEXT("🖼️ Texture detected: %s"), *Asset.GetFullName());
				HasTexture = true;
			}
		}

		if (!HasMaterial)
			UE_LOG(LogTemp, Warning, TEXT("⚠️  No materials imported for: %s"), *FileName);

		if (!HasTexture)
			UE_LOG(LogTemp, Warning, TEXT("⚠️  No textures imported for: %s"), *FileName);
	}

	UE_LOG(LogTemp, Warning, TEXT("\n=============================="));
	UE_LOG(LogTemp, Warning, TEXT("🎉 All FBX Import Tasks Completed!"));
}


void FLLMHttpServerModule::ImportGLTF()
{
	FString SourceFolder = TEXT("E:\\sourcecode\\UELLM\\model_description\\gltf\\");
	FString TargetRoot = TEXT("/Game/MyPCG/models");
	SourceFolder = FPaths::ConvertRelativePathToFull(SourceFolder);
	FPaths::NormalizeDirectoryName(SourceFolder);

	UE_LOG(LogTemp, Warning, TEXT("🔍 Searching in: %s"), *SourceFolder);

	if (!FPaths::DirectoryExists(SourceFolder))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Directory does NOT exist: %s"), *SourceFolder);
		return;
	}

	// --- 递归打印所有文件 ---
	TArray<FString> AllFiles;
	IFileManager& FileMgr = IFileManager::Get();
	FileMgr.FindFilesRecursive(AllFiles, *SourceFolder, TEXT("*.*"), true, false);

	UE_LOG(LogTemp, Warning, TEXT("📄 Listing all files in %s:"), *SourceFolder);

	TArray<FString> GLTFFiles;
	for (const FString& FilePath : AllFiles)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *FilePath);

		FString Extension = FPaths::GetExtension(FilePath, true).ToLower();
		if (Extension == TEXT(".gltf") || Extension == TEXT(".glb"))
		{
			GLTFFiles.Add(FilePath);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("📦 Found %d glTF files."), GLTFFiles.Num());
	if (GLTFFiles.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ No glTF found. Directory scanned: %s"), *SourceFolder);
		return;
	}

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	UInterchangeManager& InterchangeManager = UInterchangeManager::GetInterchangeManager();

	for (const FString& FilePath : GLTFFiles)
	{
		// 获取模型 UID（glTF 所在上一级文件夹名）
		FString ParentFolderPath = FPaths::GetPath(FilePath); // ...\0000ae10c2a24d5d90807593d6997f6f\extracted
		FString ModelUIDFolder = FPaths::GetBaseFilename(FPaths::GetPath(ParentFolderPath)); // ...\0000ae10c2a24d5d90807593d6997f6f

		// 目标文件夹下最终文件名使用模型 UID
		FString DestPath = TargetRoot + TEXT("/") + ModelUIDFolder + TEXT("/") + ModelUIDFolder;

		UE_LOG(LogTemp, Warning, TEXT("\n=============================="));
		UE_LOG(LogTemp, Warning, TEXT("🎯 Importing glTF: %s"), *FilePath);

		FString ExistingAssetPath = DestPath + TEXT(".") + ModelUIDFolder;
		if (AssetRegistryModule.Get().GetAssetByObjectPath(*ExistingAssetPath).IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("⏩ Skip (already exists): %s"), *ExistingAssetPath);
			continue;
		}

		UInterchangeSourceData* SourceData = NewObject<UInterchangeSourceData>();
		if (!SourceData->SetFilename(FilePath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to SetFilename: %s"), *FilePath);
			continue;
		}

		FImportAssetParameters ImportParams;
		ImportParams.bRunSynchronous = true;
		ImportParams.bIsAutomated = true;

		// 使用通用导入管线并设置合并网格
		UInterchangeGenericAssetsPipeline* interchangeGenericAssetsPipeline = NewObject<UInterchangeGenericAssetsPipeline>();
		if (interchangeGenericAssetsPipeline->MeshPipeline)
		{
			interchangeGenericAssetsPipeline->MeshPipeline->bCombineStaticMeshes = true;
		}

		interchangeGenericAssetsPipeline->bAssetTypeSubFolders = true;
		interchangeGenericAssetsPipeline->AssetName = ModelUIDFolder;

		ImportParams.OverridePipelines.Add(interchangeGenericAssetsPipeline);


		bool bOk = InterchangeManager.ImportScene(DestPath, SourceData, ImportParams);
		if (!bOk)
		{
			UE_LOG(LogTemp, Error, TEXT("❌ Import failed: %s"), *FilePath);
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("🎉 Import OK: %s"), *FilePath);
	}

	UE_LOG(LogTemp, Warning, TEXT("\n=== All glTF Import Tasks Completed ==="));
}
void FLLMHttpServerModule::TakeModelPicture()
{
#if WITH_EDITOR

	const FString SourcePath = TEXT("/Game/MyPCG/models");
	const FString TargetPath = TEXT("/Game/MyPCG/eval/models");

	/* ---------------- 目录 ---------------- */
	FString TargetDiskPath = FPackageName::LongPackageNameToFilename(
		TargetPath, FPackageName::GetAssetPackageExtension());
	TargetDiskPath = FPaths::GetPath(TargetDiskPath);
	IFileManager::Get().MakeDirectory(*TargetDiskPath, true);

	/* ---------------- Asset Registry ---------------- */
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.PackagePaths.Add(*SourcePath);
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;

	TArray<FAssetData> MeshAssets;
	AssetRegistryModule.Get().GetAssets(Filter, MeshAssets);
	if (MeshAssets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No StaticMesh found"));
		return;
	}

	/* ============================================================
	 * Preview Scene
	 * ============================================================ */
	FPreviewScene PreviewScene{FPreviewScene::ConstructionValues()
};
	UWorld* World = PreviewScene.GetWorld();

	/* ============================================================
	 * 灯光（必须用 Component + AddComponent）
	 * ============================================================ */

	 // Directional Light
	UDirectionalLightComponent* DirLightComp =
		NewObject<UDirectionalLightComponent>(GetTransientPackage());

	DirLightComp->Mobility = EComponentMobility::Movable;
	DirLightComp->Intensity = 5.0f;
	DirLightComp->SetWorldRotation(FRotator(-45.f, 45.f, 0.f));

	PreviewScene.AddComponent(DirLightComp, FTransform::Identity);

	// Sky Light
	USkyLightComponent* SkyLightComp =
		NewObject<USkyLightComponent>(GetTransientPackage());

	SkyLightComp->Mobility = EComponentMobility::Movable;
	SkyLightComp->Intensity = 1.0f;
	SkyLightComp->bRealTimeCapture = true;

	PreviewScene.AddComponent(SkyLightComp, FTransform::Identity);

	/* ============================================================
	 * SceneCapture
	 * ============================================================ */

	USceneCaptureComponent2D* CaptureComp =
		NewObject<USceneCaptureComponent2D>(GetTransientPackage());

	UTextureRenderTarget2D* RenderTarget =
		NewObject<UTextureRenderTarget2D>(GetTransientPackage());

	RenderTarget->InitAutoFormat(1024, 1024);
	RenderTarget->ClearColor = FLinearColor::Transparent;

	CaptureComp->TextureTarget = RenderTarget;
	CaptureComp->CaptureSource = SCS_FinalColorHDR;

	// 关闭自动捕获（非常关键）
	CaptureComp->bCaptureEveryFrame = false;
	CaptureComp->bCaptureOnMovement = false;

	// 固定曝光
	CaptureComp->PostProcessSettings.bOverride_AutoExposureMethod = true;
	CaptureComp->PostProcessSettings.AutoExposureMethod = AEM_Manual;
	CaptureComp->PostProcessSettings.bOverride_AutoExposureBias = true;
	CaptureComp->PostProcessSettings.AutoExposureBias = 1.0f;

	PreviewScene.AddComponent(CaptureComp, FTransform::Identity);

	/* ============================================================
	 * 🔥 强制构建 Scene（关键步骤 1）
	 * ============================================================ */
	World->UpdateWorldComponents(true, false);
	FlushRenderingCommands();

	// SkyLight 必须在 Flush 后再 Capture
	SkyLightComp->RecaptureSky();
	FlushRenderingCommands();

	/* ============================================================
	 * 拍摄参数
	 * ============================================================ */
	const TArray<float> YawAngles = { 0.f, 90.f, 180.f, 270.f };

	/* ============================================================
	 * 遍历 StaticMesh
	 * ============================================================ */
	for (const FAssetData& AssetData : MeshAssets)
	{
		UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
		if (!StaticMesh)
			continue;

		StaticMesh->ConditionalPostLoad();
		StaticMesh->InitResources();

		UStaticMeshComponent* MeshComp =
			NewObject<UStaticMeshComponent>(GetTransientPackage());

		MeshComp->SetStaticMesh(StaticMesh);
		MeshComp->SetMobility(EComponentMobility::Movable);

		PreviewScene.AddComponent(MeshComp, FTransform::Identity);

		const FBoxSphereBounds Bounds = StaticMesh->GetBounds();
		MeshComp->SetWorldLocation(-Bounds.Origin);

		const float Radius = Bounds.SphereRadius;
		const float CameraDistance = Radius * 2.5f;

		// Mesh 加入后，必须再 Flush 一次
		FlushRenderingCommands();

		for (float Yaw : YawAngles)
		{
			const FRotator CamRot(0.f, Yaw, 0.f);
			const FVector CamLoc =
				CamRot.RotateVector(FVector(CameraDistance, 0.f, 0.f));

			CaptureComp->SetWorldLocation(CamLoc);
			CaptureComp->SetWorldRotation(
				(FVector::ZeroVector - CamLoc).Rotation());

			/* ====================================================
			 * 🔥 正确触发捕获（关键步骤 2）
			 * ==================================================== */
			FlushRenderingCommands();
			CaptureComp->UpdateContent();
			FlushRenderingCommands();

			const FString FileName =
				FString::Printf(TEXT("%s_%03d.png"),
					*StaticMesh->GetName(), (int32)Yaw);

			const FString DiskFilePath =
				FPackageName::LongPackageNameToFilename(
					TargetPath / FileName, TEXT(""));

			if (TUniquePtr<FArchive> Ar{
				IFileManager::Get().CreateFileWriter(*DiskFilePath)
		})
			{
				FImageUtils::ExportRenderTarget2DAsPNG(RenderTarget, *Ar);
				Ar->Close();
			}
		}

		PreviewScene.RemoveComponent(MeshComp);
		FlushRenderingCommands();
	}

	UE_LOG(LogTemp, Log, TEXT("StaticMesh screenshots finished"));

#endif
}




namespace
{
	/**
	 * 使用 IFileManager 递归复制目录
	 */
	bool CopyDirectoryRecursive(
		IFileManager& FileManager,
		const FString& SrcDir,
		const FString& DstDir)
	{
		// 确保目标目录存在
		if (!FileManager.MakeDirectory(*DstDir, true))
		{
			UE_LOG(LogTemp, Error, TEXT("❌ 无法创建目录: %s"), *DstDir);
			return false;
		}

		struct FCopyVisitor : public IPlatformFile::FDirectoryVisitor
		{
			IFileManager& FileManager;
			FString SrcRoot;
			FString DstRoot;

			FCopyVisitor(IFileManager& InFM, const FString& InSrc, const FString& InDst)
				: FileManager(InFM)
				, SrcRoot(InSrc)
				, DstRoot(InDst)
			{
			}

			virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
			{
				const FString SrcPath = FilenameOrDirectory;

				// 计算相对路径
				FString RelativePath = SrcPath;
				FPaths::MakePathRelativeTo(RelativePath, *SrcRoot);

				const FString DstPath = DstRoot / RelativePath;

				if (bIsDirectory)
				{
					FileManager.MakeDirectory(*DstPath, true);
				}
				else
				{
					const uint32 Result = FileManager.Copy(
						*DstPath,
						*SrcPath,
						/*Replace=*/true,
						/*EvenIfReadOnly=*/true
					);

					if (Result != COPY_OK)
					{
						UE_LOG(LogTemp, Error, TEXT("❌ 文件复制失败: %s"), *SrcPath);
						return false;
					}
				}
				return true;
			}
		};

		FCopyVisitor Visitor(FileManager, SrcDir, DstDir);
		return FileManager.IterateDirectoryRecursively(*SrcDir, Visitor);
	}
}

void FLLMHttpServerModule::SaveDemo()
{
	IFileManager& FileManager = IFileManager::Get();

	// ============================
	// 1. 路径定义
	// ============================

	const FString DataRootDir = TEXT("E:/sourcecode/UELLM/data");

	const TArray<FString> SourceDirs =
	{
		TEXT("E:/UEProjects/ElectricDreamsEnv/Saved/PCG"),
		TEXT("E:/UEProjects/ElectricDreamsEnv/Content/MyPCG"),
		TEXT("E:/UEProjects/ElectricDreamsEnv/Plugins/LLMEditor/Source/LLMEditor/Private/LLMGenerate"),

		// ✅ 新增：模型描述 gltf 目录
		TEXT("E:/sourcecode/UELLM/model_description/gltf")
	};

	// ============================
	// 2. 确保 data 目录存在
	// ============================

	FileManager.MakeDirectory(*DataRootDir, true);

	// ============================
	// 3. 统计 demo_x 数量
	// ============================

	TArray<FString> ExistingDirs;
	FileManager.FindFiles(ExistingDirs, *(DataRootDir / TEXT("*")), false, true);

	int32 DemoCount = 0;
	for (const FString& Dir : ExistingDirs)
	{
		if (Dir.StartsWith(TEXT("demo_")))
		{
			++DemoCount;
		}
	}

	const int32 NewDemoIndex = DemoCount + 1;
	const FString DemoDirName = FString::Printf(TEXT("demo_%d"), NewDemoIndex);
	const FString DemoRootDir = DataRootDir / DemoDirName;

	FileManager.MakeDirectory(*DemoRootDir, true);

	UE_LOG(LogTemp, Display, TEXT("📁 创建 Demo: %s"), *DemoRootDir);

	// ============================
	// 4. 复制目录
	// ============================

	for (const FString& SrcDir : SourceDirs)
	{
		if (!FileManager.DirectoryExists(*SrcDir))
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠ 源目录不存在，跳过: %s"), *SrcDir);
			continue;
		}

		const FString DirName = FPaths::GetCleanFilename(SrcDir);
		const FString DstDir = DemoRootDir / DirName;

		UE_LOG(LogTemp, Display, TEXT("➡ 复制 %s -> %s"), *SrcDir, *DstDir);

		CopyDirectoryRecursive(FileManager, SrcDir, DstDir);
	}

	UE_LOG(LogTemp, Display, TEXT("✅ Demo 保存完成"));
}

void FLLMHttpServerModule::GeneratePCGGraphLog()
{

}



void FLLMHttpServerModule::GenerateRuleStruct()
{
#if WITH_EDITOR
	const FString JsonPath = TEXT("E:/UEProjects/ElectricDreamsEnv/Saved/PCG/PCGSpatialRules.json");
	const FString AssetPath = TEXT("/Game/MyPCG/MyPCGSpatialRuleAsset"); // 保存路径，含名字，不带 .uasset

	// ==========================================================
	// Step 1: 读取 JSON 文件
	// ==========================================================
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *JsonPath))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to read JSON file: %s"), *JsonPath);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to parse JSON"));
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* RulesArray = nullptr;
	if (!JsonObject->TryGetArrayField(TEXT("rules"), RulesArray) || RulesArray->Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ No 'rules' array in JSON or empty"));
		return;
	}

	// ==========================================================
	// Step 2: 创建 Package 和 Asset
	// ==========================================================
	FString PackageName = AssetPath;
	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	UPCGSpatialRuleAsset* NewAsset = NewObject<UPCGSpatialRuleAsset>(
		Package,
		*FPaths::GetBaseFilename(AssetPath),
		RF_Public | RF_Standalone
	);

	if (!NewAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to create UPCGSpatialRuleAsset"));
		return;
	}

	// ==========================================================
	// Step 3: 填充 Rules
	// ==========================================================
	for (const TSharedPtr<FJsonValue>& RuleValue : *RulesArray)
	{
		if (!RuleValue.IsValid() || RuleValue->Type != EJson::Object) continue;

		TSharedPtr<FJsonObject> RuleObj = RuleValue->AsObject();
		if (!RuleObj.IsValid()) continue;

		FPCGSpatialRule Rule;

		// ---- Slot ----
		FString SlotStr;
		if (RuleObj->TryGetStringField(TEXT("slot"), SlotStr))
		{
			if (SlotStr == "Left") Rule.Slot = EPCGDirectionSlot::Left;
			else if (SlotStr == "Right") Rule.Slot = EPCGDirectionSlot::Right;
			else if (SlotStr == "Front") Rule.Slot = EPCGDirectionSlot::Front;
			else if (SlotStr == "Back") Rule.Slot = EPCGDirectionSlot::Back;
			else if (SlotStr == "Top") Rule.Slot = EPCGDirectionSlot::Top;
			else if (SlotStr == "Bottom") Rule.Slot = EPCGDirectionSlot::Bottom;
			else if (SlotStr == "Surround") Rule.Slot = EPCGDirectionSlot::Surround;
		}

		// ---- BaseActor ----
		FString ActorName;
		if (RuleObj->TryGetStringField(TEXT("base_actor"), ActorName) && !ActorName.IsEmpty())
		{
			FString ActorPath = FString::Printf(TEXT("/Game/MyActors/%s.%s"), *ActorName, *ActorName);
			Rule.BaseActor = TSoftObjectPtr<AActor>(FSoftObjectPath(ActorPath));
		}

		// ---- Offset ----
		const TSharedPtr<FJsonObject>* OffsetObjPtr = nullptr;
		if (RuleObj->TryGetObjectField(TEXT("offset"), OffsetObjPtr) && OffsetObjPtr && OffsetObjPtr->IsValid())
		{
			const TSharedPtr<FJsonObject>& OffsetObj = *OffsetObjPtr;
			Rule.Offset.X = OffsetObj->GetNumberField(TEXT("x"));
			Rule.Offset.Y = OffsetObj->GetNumberField(TEXT("y"));
			Rule.Offset.Z = OffsetObj->GetNumberField(TEXT("z"));
		}

		// ---- Extent ----
		const TSharedPtr<FJsonObject>* ExtentObjPtr = nullptr;
		if (RuleObj->TryGetObjectField(TEXT("extent"), ExtentObjPtr) && ExtentObjPtr && ExtentObjPtr->IsValid())
		{
			const TSharedPtr<FJsonObject>& ExtentObj = *ExtentObjPtr;
			Rule.Extent.X = ExtentObj->GetNumberField(TEXT("x"));
			Rule.Extent.Y = ExtentObj->GetNumberField(TEXT("y"));
			Rule.Extent.Z = ExtentObj->GetNumberField(TEXT("z"));
		}


		// ---- Count ----
		double CountValue;
		if (RuleObj->TryGetNumberField(TEXT("count"), CountValue))
		{
			Rule.Count = static_cast<int32>(CountValue);
		}

		NewAsset->Rules.Add(Rule);
	}

	// ==========================================================
	// Step 4: 标记 Dirty 并保存 Package
	// ==========================================================
	NewAsset->MarkPackageDirty();

	FString FilePath = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bForceByteSwapping = false;
	SaveArgs.SaveFlags = SAVE_NoError; // 也可以根据需要设置 SAVE_Async | SAVE_KeepGUID 等
	// SaveArgs.bWarnOfLongFilename = true; // 可选
	// SaveArgs.bSlowTask = true; // 可选


	bool bSuccess = UPackage::SavePackage(
		Package,
		NewAsset,
		*FilePath,
		SaveArgs
	);

	UE_LOG(LogTemp, Log, TEXT("PCGSpatialRuleAsset saved to %s: %s"), *FilePath, bSuccess ? TEXT("✅ Success") : TEXT("❌ Failed"));
#endif // WITH_EDITOR
}




void FLLMHttpServerModule::CreatePCGGraphNodesFromJSON(
	const FString& JsonFilePath_Nodes,
	const FString& JsonFilePath_Connections)
{
	// ==========================================================
	// Step 0. 读取节点定义 JSON
	// ==========================================================
	FString JsonContent_Nodes;
	if (!FFileHelper::LoadFileToString(JsonContent_Nodes, *JsonFilePath_Nodes))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法读取节点 JSON 文件: %s"), *JsonFilePath_Nodes);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject_Nodes;
	TSharedRef<TJsonReader<>> ReaderNodes = TJsonReaderFactory<>::Create(JsonContent_Nodes);
	if (!FJsonSerializer::Deserialize(ReaderNodes, JsonObject_Nodes) || !JsonObject_Nodes.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 节点 JSON 解析失败"));
		return;
	}

	// ==========================================================
	// Step 1. 读取连接定义 JSON
	// ==========================================================
	FString JsonContent_Connections;
	if (!FFileHelper::LoadFileToString(JsonContent_Connections, *JsonFilePath_Connections))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法读取连接 JSON 文件: %s"), *JsonFilePath_Connections);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject_Connections;
	TSharedRef<TJsonReader<>> ReaderConnections = TJsonReaderFactory<>::Create(JsonContent_Connections);
	if (!FJsonSerializer::Deserialize(ReaderConnections, JsonObject_Connections) || !JsonObject_Connections.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 连接 JSON 解析失败"));
		return;
	}

	// ==========================================================
	// Step 2. 创建 PCGGraph 资产
	// ==========================================================
	FString GraphName = TEXT("GeneratedGraph");
	if (JsonObject_Nodes->HasField("graph_name"))
	{
		GraphName = JsonObject_Nodes->GetStringField("graph_name");
	}

	const FString PackageName = TEXT("/Game/PCG/") + GraphName;
	const FString AssetName = GraphName;

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	UPCGGraph* PCGGraph = NewObject<UPCGGraph>(Package, UPCGGraph::StaticClass(), *AssetName, RF_Public | RF_Standalone);
	if (!PCGGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法创建 PCGGraph"));
		return;
	}

	// ==========================================================
	// Step 3. 创建节点并保存映射
	// ==========================================================
	const TArray<TSharedPtr<FJsonValue>>* WorkflowsArray;
	if (!JsonObject_Nodes->TryGetArrayField(TEXT("recommended_workflows"), WorkflowsArray))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ JSON 中缺少 recommended_workflows 字段"));
		return;
	}

	// 模型名 -> 节点名 -> 节点指针
	TMap<FString, TMap<FString, UPCGNode*>> ModelNodeMap;
	int32 TotalNodeCount = 0;

	for (const TSharedPtr<FJsonValue>& WorkflowVal : *WorkflowsArray)
	{
		TSharedPtr<FJsonObject> WorkflowObj = WorkflowVal->AsObject();
		if (!WorkflowObj.IsValid())
			continue;

		FString ModelName = WorkflowObj->GetStringField("model");
		const TArray<TSharedPtr<FJsonValue>>* NodeArray;

		if (!WorkflowObj->TryGetArrayField("nodes", NodeArray))
			continue;

		for (const TSharedPtr<FJsonValue>& NodeVal : *NodeArray)
		{
			TSharedPtr<FJsonObject> NodeObj = NodeVal->AsObject();
			if (!NodeObj.IsValid())
				continue;

			FString NodeType = NodeObj->GetStringField("name");

			// 查找 Settings 类
			UClass* SettingsClass = nullptr;
			for (TObjectIterator<UClass> It; It; ++It)
			{
				if (It->IsChildOf(UPCGSettings::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
				{
					if (It->GetName().Contains(NodeType))
					{
						SettingsClass = *It;
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

			// 设置位置
			if (NodeObj->HasField("position"))
			{
				const TArray<TSharedPtr<FJsonValue>>* PosArray;
				if (NodeObj->TryGetArrayField("position", PosArray) && PosArray->Num() >= 2)
				{
					float X = static_cast<float>((*PosArray)[0]->AsNumber());
					float Y = static_cast<float>((*PosArray)[1]->AsNumber());
					Node->SetNodePosition(X, Y);
				}
			}

			ModelNodeMap.FindOrAdd(ModelName).Add(NodeType, Node);
			TotalNodeCount++;
		}
	}

	// ==========================================================
	// Step 4. 创建连接（采用 OutPin->AddEdgeTo(InPin)）
	// ==========================================================
	const TArray<TSharedPtr<FJsonValue>>* JsonConnections;
	if (JsonObject_Connections->TryGetArrayField("connections", JsonConnections))
	{
		for (const TSharedPtr<FJsonValue>& ConnVal : *JsonConnections)
		{
			TSharedPtr<FJsonObject> ConnObj = ConnVal->AsObject();
			if (!ConnObj.IsValid()) continue;

			FString ModelName = ConnObj->GetStringField("model");
			FString FromNodeName = ConnObj->GetStringField("from_node");
			FString ToNodeName = ConnObj->GetStringField("to_node");
			FString FromPinName = ConnObj->GetStringField("from_pin");
			FString ToPinName = ConnObj->GetStringField("to_pin");

			if (!ModelNodeMap.Contains(ModelName))
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 模型 %s 未在节点定义中出现，跳过连接"), *ModelName);
				continue;
			}

			TMap<FString, UPCGNode*>& NodeMap = ModelNodeMap[ModelName];
			UPCGNode* FromNode = NodeMap.FindRef(FromNodeName);
			UPCGNode* ToNode = NodeMap.FindRef(ToNodeName);

			if (!FromNode || !ToNode)
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 找不到连接节点: %s 或 %s"), *FromNodeName, *ToNodeName);
				continue;
			}

			if (UPCGPin* OutPin = FromNode->GetOutputPin(FName(*FromPinName)))
			{
				if (UPCGPin* InPin = ToNode->GetInputPin(FName(*ToPinName)))
				{
					OutPin->AddEdgeTo(InPin);
					UE_LOG(LogTemp, Display, TEXT("🔗 已连接 [%s.%s -> %s.%s]"), *FromNodeName, *FromPinName, *ToNodeName, *ToPinName);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠️ 未找到输入引脚 %s.%s"), *ToNodeName, *ToPinName);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 未找到输出引脚 %s.%s"), *FromNodeName, *FromPinName);
			}
		}
	}

	// ==========================================================
	// Step 5. 注册与保存
	// ==========================================================
	FAssetRegistryModule::AssetCreated(PCGGraph);
	PCGGraph->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.Error = GWarn;

	if (UPackage::SavePackage(Package, PCGGraph, *PackageFileName, SaveArgs))
	{
		UE_LOG(LogTemp, Display, TEXT("✅ 成功保存 PCG Graph [%s]，共创建 %d 个节点"), *AssetName, TotalNodeCount);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 保存 PCG Graph 失败"));
	}
}


void FLLMHttpServerModule::GenerateBlueprintsFromSpawnActorJSON(
	const FString& SpawnJsonPath,
	const FString& SelectedModelJsonPath,
	const FString& InteractiveAnalyzerJsonPath
)
{
	// ======================= 1. Spawn JSON =======================
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *SpawnJsonPath)) return;

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	if (!FJsonSerializer::Deserialize(
		TJsonReaderFactory<>::Create(JsonContent), JsonArray))
	{
		return;
	}

	// ======================= 2. SelectedModel =======================
	FString Json2Content;
	if (!FFileHelper::LoadFileToString(Json2Content, *SelectedModelJsonPath)) return;

	TSharedPtr<FJsonObject> SelectedRoot;
	if (!FJsonSerializer::Deserialize(
		TJsonReaderFactory<>::Create(Json2Content), SelectedRoot))
	{
		return;
	}

	TMap<FString, FString> ModelToUID;
	const TArray<TSharedPtr<FJsonValue>>* SelectedArr = nullptr;
	if (SelectedRoot->TryGetArrayField(TEXT("selected_models"), SelectedArr))
	{
		for (auto& V : *SelectedArr)
		{
			auto O = V->AsObject();
			if (!O.IsValid()) continue;

			FString Name, UID;
			O->TryGetStringField(TEXT("model_name"), Name);
			O->TryGetStringField(TEXT("selected_uid"), UID);

			if (!Name.IsEmpty() && !UID.IsEmpty())
			{
				ModelToUID.Add(Name, UID);
			}
		}
	}

	// ======================= 3. Analyzer =======================
	FString AnalyzerJson;
	if (!FFileHelper::LoadFileToString(AnalyzerJson, *InteractiveAnalyzerJsonPath)) return;

	TSharedPtr<FJsonObject> AnalyzerRoot;
	if (!FJsonSerializer::Deserialize(
		TJsonReaderFactory<>::Create(AnalyzerJson), AnalyzerRoot))
	{
		return;
	}

	TMap<FString, bool> ModelToAI;
	const TArray<TSharedPtr<FJsonValue>>* InteractiveArr = nullptr;
	if (AnalyzerRoot->TryGetArrayField(TEXT("interactive_objects"), InteractiveArr))
	{
		for (auto& V : *InteractiveArr)
		{
			auto O = V->AsObject();
			if (!O.IsValid()) continue;

			FString Name;
			bool bAI = false;
			O->TryGetStringField(TEXT("model_name"), Name);
			O->TryGetBoolField(TEXT("is_movable_ai_unit"), bAI);

			if (!Name.IsEmpty())
			{
				ModelToAI.Add(Name, bAI);
			}
		}
	}

	// ======================= 4. 收集 TemplateActor =======================
	TSet<FString> TemplateClasses;
	for (auto& V : JsonArray)
	{
		auto O = V->AsObject();
		if (!O.IsValid()) continue;

		const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
		if (!O->TryGetArrayField(TEXT("nodes"), Nodes)) continue;

		for (auto& N : *Nodes)
		{
			auto NO = N->AsObject();
			if (!NO.IsValid()) continue;

			FString Type;
			NO->TryGetStringField(TEXT("type"), Type);
			if (!Type.Contains(TEXT("PCGSpawnActorSettings"))) continue;

			auto Params = NO->GetObjectField(TEXT("parameters"));
			FString TemplateActor;
			if (Params->TryGetStringField(TEXT("TemplateActor"), TemplateActor))
			{
				if (!TemplateActor.IsEmpty())
				{
					TemplateClasses.Add(TemplateActor);
				}
			}
		}
	}

	// ======================= 5. AssetRegistry =======================
	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	const FString OutputFolder = TEXT("/Game/MyPCG/InteractiveObjectBluePrints");

	// ======================= 6. 生成蓝图 =======================
	for (const FString& ClassName : TemplateClasses)
	{
		const FString ParentName =
			ClassName.StartsWith(TEXT("A")) ? ClassName.RightChop(1) : ClassName;

		const FString ModelName =
			ParentName.Replace(TEXT("InteractiveObject"), TEXT(""));

		const bool bIsAI = ModelToAI.FindRef(ModelName);
		const FString UID = ModelToUID.FindRef(ModelName);
		if (UID.IsEmpty()) continue;

		// ---------- 查找 Mesh ----------
		UObject* MeshAsset = nullptr;

		FARFilter Filter;
		Filter.ClassPaths = {
			UStaticMesh::StaticClass()->GetClassPathName(),
			USkeletalMesh::StaticClass()->GetClassPathName()
		};
		Filter.PackagePaths.Add(TEXT("/Game/MyPCG/models"));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> Assets;
		Registry.GetAssets(Filter, Assets);

		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString() == UID)
			{
				MeshAsset = Asset.GetAsset();
				break;
			}
		}

		// ---------- ParentClass ----------
		UClass* ParentClass = nullptr;

		if (bIsAI)
		{
			if (UBlueprint* ShooterBP = Cast<UBlueprint>(
				StaticLoadObject(
					UBlueprint::StaticClass(),
					nullptr,
					TEXT("/Game/Variant_Shooter/Blueprints/AI/BP_ShooterNPC.BP_ShooterNPC"))))
			{
				ParentClass = ShooterBP->GeneratedClass;
			}
		}
		else
		{
			ParentClass = FindObject<UClass>(ANY_PACKAGE, *ParentName);
		}
		if (!ParentClass) continue;

		// ---------- Create Blueprint ----------
		const FString BPName = TEXT("BP_") + ParentName;
		const FString PackagePath = OutputFolder + TEXT("/") + BPName;

		UPackage* Package = CreatePackage(*PackagePath);
		Package->FullyLoad();

		UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(
			ParentClass,
			Package,
			FName(*BPName),
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass()
		);
		if (!BP) continue;
		if (MeshAsset)
		{
			// ======================= ⭐ 核心：CDO 设置 Mesh + 精确碰撞 =======================
			if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(BP->GeneratedClass))
			{
				if (AActor* CDO = Cast<AActor>(BPClass->GetDefaultObject()))
				{
					const bool bIsStaticMesh = MeshAsset->IsA<UStaticMesh>();
					const bool bIsSkeletalMesh = MeshAsset->IsA<USkeletalMesh>();

					TArray<UActorComponent*> Components;
					CDO->GetComponents(Components);

					for (UActorComponent* Comp : Components)
					{
						if (auto* SMC = Cast<UStaticMeshComponent>(Comp))
						{
							SMC->SetStaticMesh(bIsStaticMesh ? Cast<UStaticMesh>(MeshAsset) : nullptr);

							//// ---------- 精确碰撞设置 ----------
							//SMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
							//SMC->SetCollisionObjectType(ECC_WorldDynamic);
							//SMC->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

							//// UE5 精确碰撞：通过 BodySetup
							//if (SMC->GetStaticMesh())
							//{
							//	if (UBodySetup* BodySetup = SMC->GetStaticMesh()->GetBodySetup())
							//	{
							//		BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
							//		BodySetup->bGenerateMirroredCollision = false; // 可选，根据需求
							//	}

							//	// 刷新碰撞应用
							//	SMC->RecreatePhysicsState();
							//}
						}
						else if (auto* SKC = Cast<USkeletalMeshComponent>(Comp))
						{
							SKC->SetSkeletalMesh(bIsSkeletalMesh ? Cast<USkeletalMesh>(MeshAsset) : nullptr);

							//// ---------- 精确碰撞设置 ----------
							//SKC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
							//SKC->SetCollisionObjectType(ECC_WorldDynamic);
							//SKC->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);


						}
					}
				}
			}
		}

		// ---------- Save ----------
		FAssetRegistryModule::AssetCreated(BP);
		Package->MarkPackageDirty();

		const FString Filename =
			FPackageName::LongPackageNameToFilename(
				PackagePath,
				FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GWarn;

		UPackage::SavePackage(Package, BP, *Filename, SaveArgs);
	}

	UE_LOG(LogTemp, Display, TEXT("🎉 蓝图全部生成完毕！（CDO 方式，精确碰撞 + 与 Pawn 碰撞）"));
}








void FLLMHttpServerModule::LinkPCGGraphNodesFromJSON(const FString& JsonFilePath)
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

	// 蓝图所在目录（用于加载已生成的 BP 类）
	const FString BlueprintFolder = TEXT("/Game/MyPCG/InteractiveObjectBluePrints");

	// ==========================================================
	// 创建 Package 和 Graph
	// ==========================================================
	const FString GraphName = TEXT("ComposedPCGGraph1");
	const FString PackageName = TEXT("/Game/MyPCG/") + GraphName;
	const FString AssetName = GraphName;

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	UPCGGraph* PCGGraph = NewObject<UPCGGraph>(Package, UPCGGraph::StaticClass(), *AssetName, RF_Public | RF_Standalone);
	if (!PCGGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法创建 PCGGraph"));
		return;
	}

	// ==========================================================
	// Step 1. 创建节点（记录每个模型的起始全局索引）
	// ==========================================================
	TMap<FString, UPCGNode*> NodeMap;
	TArray<int32> ModelStartIndices; // 对应 JsonArray 的索引，记录每个模型节点的全局起始索引
	int32 GlobalNodeIndex = 0; // 全局节点索引

	for (int32 ModelIdx = 0; ModelIdx < JsonArray.Num(); ++ModelIdx)
	{
		const TSharedPtr<FJsonValue>& ModelVal = JsonArray[ModelIdx];
		TSharedPtr<FJsonObject> ModelObj = ModelVal->AsObject();
		if (!ModelObj.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ JsonArray[%d] 不是对象，跳过"), ModelIdx);
			ModelStartIndices.Add(-1);
			continue;
		}

		// 记录本模型的起始全局索引
		ModelStartIndices.Add(GlobalNodeIndex);

		const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
		if (!ModelObj->TryGetArrayField(TEXT("nodes"), NodesArray))
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ JsonArray[%d] 没有 nodes 字段，跳过"), ModelIdx);
			continue;
		}

		for (const TSharedPtr<FJsonValue>& NodeVal : *NodesArray)
		{
			TSharedPtr<FJsonObject> NodeObj = NodeVal->AsObject();
			if (!NodeObj.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 某个 node 不是对象，跳过"));
				continue;
			}

			// 获取 type 和 parameters（可能不存在 parameters）
			FString NodeType;
			if (!NodeObj->TryGetStringField(TEXT("type"), NodeType))
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ node 未包含 type 字段，跳过"));
				continue;
			}
			TSharedPtr<FJsonObject> Params = nullptr;
			if (NodeObj->HasField(TEXT("parameters")))
			{
				Params = NodeObj->GetObjectField(TEXT("parameters"));
			}

			// 查找对应 Settings 类（匹配类名包含 NodeType 的逻辑）
			UClass* SettingsClass = nullptr;
			for (TObjectIterator<UClass> It; It; ++It)
			{
				UClass* Class = *It;
				if (Class->IsChildOf(UPCGSettings::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
				{
					if (NodeType.Contains(Class->GetName()))
					{
						SettingsClass = Class;
						break;
					}
				}
			}

			if (!SettingsClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 找不到对应的 PCGSettings 类: %s"), *NodeType);
				// 仍然增加全局索引占位（保持全局编号与 JSON 的局部编号对齐）
				FString MissingNodeId = FString::FromInt(GlobalNodeIndex);
				NodeMap.Add(MissingNodeId, nullptr);
				GlobalNodeIndex++;
				continue;
			}

			// 创建 Settings 实例并添加节点
			UPCGSettings* Settings = NewObject<UPCGSettings>(PCGGraph, SettingsClass);
			UPCGNode* Node = PCGGraph->AddNode(Settings);

			// 解析位置 @[x,y] 并设置
			FString TypePart, PosPart;
			if (NodeType.Split(TEXT("@["), &TypePart, &PosPart))
			{
				PosPart = PosPart.LeftChop(1); // 去除末尾 ']'
				TArray<FString> XY;
				PosPart.ParseIntoArray(XY, TEXT(","), true);
				if (XY.Num() == 2)
				{
					float X = FCString::Atof(*XY[0]);
					float Y = FCString::Atof(*XY[1]);
					Node->SetNodePosition(X, Y);
				}
			}

			// 为避免 Params 为空的情况，先判断
			if (Params.IsValid())
			{
				for (const auto& ParamPair : Params->Values)
				{
					const FString& ParamKey = ParamPair.Key;
					const FName ParamName(*ParamKey);

					FProperty* Property = Settings->GetClass()->FindPropertyByName(ParamName);
					if (!Property)
					{
						UE_LOG(LogTemp, Warning, TEXT("⚠️ 在 %s 中找不到属性: %s"), *Settings->GetClass()->GetName(), *ParamKey);
						continue;
					}

					const TSharedPtr<FJsonValue>& JsonVal = ParamPair.Value;
					//void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Settings);

					// -------------------- 安全处理：PCGSpawnActorSettings 的 TemplateActor -> TemplateActorClass --------------------
					if (Settings->IsA(UPCGSpawnActorSettings::StaticClass()) && ParamKey.Equals(TEXT("TemplateActor"), ESearchCase::IgnoreCase))
					{
						// TemplateActor in JSON is like "AChestInteractiveObject"
						if (JsonVal->Type != EJson::String)
						{
							UE_LOG(LogTemp, Warning, TEXT("⚠️ TemplateActor 值不是字符串，跳过"));
							continue;
						}

						FString TemplateActorStr = JsonVal->AsString();
						if (TemplateActorStr.IsEmpty())
						{
							UE_LOG(LogTemp, Warning, TEXT("⚠️ TemplateActor 为空字符串，跳过"));
							continue;
						}

						// 去掉 A 前缀
						FString PureName = TemplateActorStr;
						if (PureName.StartsWith(TEXT("A")))
						{
							PureName = PureName.RightChop(1);
						}

						// 构造 BP 名称和路径
						FString BPBaseName = FString::Printf(TEXT("BP_%s"), *PureName);
						FString BPRuntimePath = FString::Printf(TEXT("%s/%s.%s_C"), *BlueprintFolder, *BPBaseName, *BPBaseName);
						FString BPAssetPath = FString::Printf(TEXT("%s/%s.%s"), *BlueprintFolder, *BPBaseName, *BPBaseName);

						// 尝试先加载运行时生成类（带 _C 后缀）
						UClass* BPClass = LoadObject<UClass>(nullptr, *BPRuntimePath);
						// 若失败，再尝试加载 UBlueprint 资源并取其 GeneratedClass
						if (!BPClass)
						{
							UBlueprint* BPAsset = LoadObject<UBlueprint>(nullptr, *BPAssetPath);
							if (BPAsset && BPAsset->GeneratedClass)
							{
								BPClass = BPAsset->GeneratedClass;
							}
						}

						if (!BPClass)
						{
							UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法加载蓝图类（既没有 runtime _C，也没有 UBlueprint）：%s 或 %s"), *BPRuntimePath, *BPAssetPath);
							continue;
						}

						// 验证 BPClass 确实是 Actor 子类
						if (!BPClass->IsChildOf(AActor::StaticClass()))
						{
							UE_LOG(LogTemp, Warning, TEXT("⚠️ 加载到的类不是 AActor 子类：%s"), *BPClass->GetName());
							continue;
						}

						// 直接通过反射写入 TemplateActorClass 字段（避免调用可能触发编辑器事件的 SetTemplateActorClass）
						// 找到 TemplateActorClass 属性（受保护），名称为 TemplateActorClass
						FProperty* TemplateClassProp = Settings->GetClass()->FindPropertyByName(FName(TEXT("TemplateActorClass")));
						if (!TemplateClassProp)
						{
							UE_LOG(LogTemp, Warning, TEXT("⚠️ 未找到 TemplateActorClass 属性（类 %s）"), *Settings->GetClass()->GetName());
							continue;
						}

						// 如果是 FClassProperty，则直接设置
						if (FClassProperty* ClassProp = CastField<FClassProperty>(TemplateClassProp))
						{
							// set into Settings instance
							ClassProp->SetPropertyValue_InContainer(Settings, BPClass);

							// 还尝试设置 bAllowTemplateActorEditing = false（如果存在此字段）
							FProperty* AllowEditProp = Settings->GetClass()->FindPropertyByName(FName(TEXT("bAllowTemplateActorEditing")));
							if (FBoolProperty* BoolProp = CastField<FBoolProperty>(AllowEditProp))
							{
								BoolProp->SetPropertyValue_InContainer(Settings, false);
							}

							UE_LOG(LogTemp, Display, TEXT("✅ 为节点 %s 设置 TemplateActorClass -> %s"), *NodeType, *BPClass->GetName());
						}
						else
						{
							// 有的版本 TemplateActorClass 可能以不同类型存在，尝试处理 SoftClassPath (FSoftClassPath 在 StructProperty 中)
							if (FStructProperty* StructProp = CastField<FStructProperty>(TemplateClassProp))
							{
								UScriptStruct* StructType = StructProp->Struct;
								if (StructType && StructType->GetFName() == FName(TEXT("SoftClassPath")))
								{
									// 把 BPClass 的路径写入 FSoftClassPath
									FSoftClassPath* DestSoft = StructProp->ContainerPtrToValuePtr<FSoftClassPath>(Settings);
									if (DestSoft)
									{
										*DestSoft = FSoftClassPath(BPClass->GetPathName());
										UE_LOG(LogTemp, Display, TEXT("✅ 为节点 %s 设置 SoftClassPath TemplateActorClass -> %s"), *NodeType, *BPClass->GetPathName());
										// 也尝试关闭编辑开关
										FProperty* AllowEditProp = Settings->GetClass()->FindPropertyByName(FName(TEXT("bAllowTemplateActorEditing")));
										if (FBoolProperty* BoolProp = CastField<FBoolProperty>(AllowEditProp))
										{
											BoolProp->SetPropertyValue_InContainer(Settings, false);
										}
									}
									else
									{
										UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法设置 SoftClassPath 的目的地址"));
									}
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("⚠️ TemplateActorClass 属性既不是 FClassProperty 也不是 SoftClassPath，类型: %s"), *StructType->GetName());
								}
							}
							else
							{
								UE_LOG(LogTemp, Warning, TEXT("⚠️ TemplateActorClass 属性类型不可处理: %s"), *TemplateClassProp->GetClass()->GetName());
							}
						}

						// 跳过后续对 TemplateActor 的常规赋值处理
						continue;
					}

					// ---------- 通用属性处理（保持原行为） ----------
					void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Settings);

					// 数值类型（整数/浮点）
					if (FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
					{
						if (JsonVal->Type == EJson::Number)
						{
							double Num = JsonVal->AsNumber();
							if (NumProp->IsInteger())
								NumProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(Num));
							else
								NumProp->SetFloatingPointPropertyValue(ValuePtr, Num);
						}
						else if (JsonVal->Type == EJson::String)
						{
							double Num = FCString::Atod(*JsonVal->AsString());
							if (NumProp->IsInteger())
								NumProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(Num));
							else
								NumProp->SetFloatingPointPropertyValue(ValuePtr, Num);
						}
					}
					// 布尔
					else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
					{
						if (JsonVal->Type == EJson::Boolean)
							BoolProp->SetPropertyValue(ValuePtr, JsonVal->AsBool());
					}
					// 字符串
					else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
					{
						if (JsonVal->Type == EJson::String)
							StrProp->SetPropertyValue(ValuePtr, JsonVal->AsString());
					}
					// FName
					else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
					{
						if (JsonVal->Type == EJson::String)
							NameProp->SetPropertyValue(ValuePtr, FName(*JsonVal->AsString()));
					}
					// 数组（简单数值或字符串数组）-- 可扩展
					else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
					{
						const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
						if (JsonVal->TryGetArray(Arr))
						{
							FScriptArrayHelper Helper(ArrayProp, ValuePtr);
							Helper.EmptyValues();

							FProperty* InnerProp = ArrayProp->Inner;
							if (FNumericProperty* InnerNum = CastField<FNumericProperty>(InnerProp))
							{
								for (const auto& Elem : *Arr)
								{
									int32 NewIndex = Helper.AddValue();
									void* Dest = Helper.GetRawPtr(NewIndex);
									double Val = Elem->AsNumber();
									if (InnerNum->IsInteger())
										InnerNum->SetIntPropertyValue(Dest, static_cast<int64>(Val));
									else
										InnerNum->SetFloatingPointPropertyValue(Dest, Val);
								}
							}
							else if (FStrProperty* InnerStr = CastField<FStrProperty>(InnerProp))
							{
								for (const auto& Elem : *Arr)
								{
									int32 NewIndex = Helper.AddValue();
									InnerStr->SetPropertyValue(Helper.GetRawPtr(NewIndex), Elem->AsString());
								}
							}
						}
					}
					// Struct 类型（只实现 FVector 的简单处理，如需更多结构体支持可扩展）
					else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
					{
						if (StructProp->Struct->GetFName() == NAME_Vector)
						{
							const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
							if (JsonVal->TryGetArray(Arr) && Arr->Num() >= 3)
							{
								FVector Vec((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
								FVector* Dest = StructProp->ContainerPtrToValuePtr<FVector>(Settings);
								*Dest = Vec;
							}
						}
						else
						{
							// 可扩充：支持嵌套 Struct JSON 对象解析
						}
					}
					else
					{
						UE_LOG(LogTemp, Verbose, TEXT("⚠️ 未处理的属性类型: %s"), *Property->GetClass()->GetName());
					}
				} // end for params
			} // end if Params valid

			// 将节点加入 NodeMap 使用全局索引作为 key
			FString NodeId = FString::FromInt(GlobalNodeIndex);
			NodeMap.Add(NodeId, Node);
			GlobalNodeIndex++;
		} // end nodes loop
	} // end models loop

	// ==========================================================
	// Step 2. 创建连接（使用每个模型的起始索引进行局部->全局转换）
	// ==========================================================
	int32 CurrentModelIdx = 0;
	int32 AccumulatedNodesCount = 0; // 备用：用于简单校验

	for (const TSharedPtr<FJsonValue>& ModelVal : JsonArray)
	{
		TSharedPtr<FJsonObject> ModelObj = ModelVal->AsObject();
		if (!ModelObj.IsValid())
		{
			++CurrentModelIdx;
			continue;
		}

		int32 ModelStart = (ModelStartIndices.IsValidIndex(CurrentModelIdx) ? ModelStartIndices[CurrentModelIdx] : -1);
		if (ModelStart < 0)
		{
			++CurrentModelIdx;
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray = nullptr;
		if (!ModelObj->TryGetArrayField(TEXT("connections"), ConnectionsArray))
		{
			++CurrentModelIdx;
			continue;
		}

		for (const TSharedPtr<FJsonValue>& ConnVal : *ConnectionsArray)
		{
			if (!ConnVal.IsValid() || ConnVal->Type != EJson::String) continue;

			FString ConnStr = ConnVal->AsString(); // like "0.Out|1.In"
			FString FromPart, ToPart;
			if (!ConnStr.Split(TEXT("|"), &FromPart, &ToPart))
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ connections 格式错误: %s"), *ConnStr);
				continue;
			}

			// Parse "0.Out" -> local index "0" and pin "Out"
			FString FromLocalIdxStr, FromPinName;
			FString ToLocalIdxStr, ToPinName;
			if (!FromPart.Split(TEXT("."), &FromLocalIdxStr, &FromPinName) || !ToPart.Split(TEXT("."), &ToLocalIdxStr, &ToPinName))
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ connection 子项解析失败: %s"), *ConnStr);
				continue;
			}

			// 将局部索引转换为全局索引
			int32 FromLocalIdx = FCString::Atoi(*FromLocalIdxStr);
			int32 ToLocalIdx = FCString::Atoi(*ToLocalIdxStr);

			int32 FromGlobalIdx = ModelStart + FromLocalIdx;
			int32 ToGlobalIdx = ModelStart + ToLocalIdx;

			FString FromGlobalId = FString::FromInt(FromGlobalIdx);
			FString ToGlobalId = FString::FromInt(ToGlobalIdx);

			UPCGNode** FromNodePtr = NodeMap.Find(FromGlobalId);
			UPCGNode** ToNodePtr = NodeMap.Find(ToGlobalId);

			if (!FromNodePtr || !ToNodePtr)
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 连接失败：找不到节点全局ID %s 或 %s (原始: %s)"), *FromGlobalId, *ToGlobalId, *ConnStr);
				continue;
			}

			UPCGNode* FromNode = *FromNodePtr;
			UPCGNode* ToNode = *ToNodePtr;
			if (!FromNode || !ToNode)
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 连接节点为空指针: %s 或 %s"), *FromGlobalId, *ToGlobalId);
				continue;
			}

			// 查找并连接 pin
			FName FromPinFName(*FromPinName);
			FName ToPinFName(*ToPinName);

			if (UPCGPin* OutPin = FromNode->GetOutputPin(FromPinFName))
			{
				if (UPCGPin* InPin = ToNode->GetInputPin(ToPinFName))
				{
					OutPin->AddEdgeTo(InPin);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠️ 未找到输入引脚 %s 在节点 %s"), *ToPinName, *ToGlobalId);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠️ 未找到输出引脚 %s 在节点 %s"), *FromPinName, *FromGlobalId);
			}
		} // end connections loop

		++CurrentModelIdx;
	} // end models loop

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


//void FLLMHttpServerModule::GeneratePCGGraphLogFromJSON()
//{
//	const FString& JsonFilePath = FPaths::ProjectSavedDir() / TEXT("PCG/ComposedGraph.json");
//	// ==========================================================
//	// 日志系统初始化
//	// ==========================================================
//	FString LogBuffer;
//
//	auto AppendLog = [&](const FString& Level, const FString& Msg)
//		{
//			LogBuffer += FString::Printf(TEXT("[%s] %s\n"), *Level, *Msg);
//		};
//
//	AppendLog(TEXT("INFO"), FString::Printf(TEXT("开始构建 PCG Graph, JSON 路径: %s"), *JsonFilePath));
//
//	// ==========================================================
//	// 读取 JSON
//	// ==========================================================
//	FString JsonContent;
//	if (!FFileHelper::LoadFileToString(JsonContent, *JsonFilePath))
//	{
//		UE_LOG(LogTemp, Error, TEXT("❌ 无法读取 JSON 文件: %s"), *JsonFilePath);
//		AppendLog(TEXT("ERROR"), TEXT("无法读取 JSON 文件"));
//		return;
//	}
//
//	TArray<TSharedPtr<FJsonValue>> JsonArray;
//	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
//
//	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
//	{
//		UE_LOG(LogTemp, Error, TEXT("❌ JSON 解析失败或为空"));
//		AppendLog(TEXT("ERROR"), TEXT("JSON 解析失败或为空"));
//		return;
//	}
//	// 蓝图所在目录（用于加载已生成的 BP 类）
//	const FString BlueprintFolder = TEXT("/Game/MyPCG/InteractiveObjectBluePrints");
//	AppendLog(TEXT("INFO"), FString::Printf(TEXT("JSON 解析成功，模型数量: %d"), JsonArray.Num()));
//
//	// ==========================================================
//	// 创建 Package 和 Graph
//	// ==========================================================
//	const FString GraphName = TEXT("LogComposedPCGGraph");
//	const FString PackageName = TEXT("/Game/MyPCG/") + GraphName;
//
//	UPackage* Package = CreatePackage(*PackageName);
//	Package->FullyLoad();
//
//	UPCGGraph* PCGGraph = NewObject<UPCGGraph>(
//		Package,
//		UPCGGraph::StaticClass(),
//		*GraphName,
//		RF_Public | RF_Standalone
//	);
//
//	if (!PCGGraph)
//	{
//		UE_LOG(LogTemp, Error, TEXT("❌ 无法创建 PCGGraph"));
//		AppendLog(TEXT("ERROR"), TEXT("无法创建 PCGGraph"));
//		return;
//	}
//
//	AppendLog(TEXT("INFO"), TEXT("PCGGraph 创建成功"));
//
//	// ==========================================================
//	// Step 1. 创建节点
//	// ==========================================================
//	TMap<FString, UPCGNode*> NodeMap;
//	TArray<int32> ModelStartIndices;
//	int32 GlobalNodeIndex = 0;
//
//	for (int32 ModelIdx = 0; ModelIdx < JsonArray.Num(); ++ModelIdx)
//	{
//		AppendLog(TEXT("INFO"), FString::Printf(TEXT("---- 开始处理 Model %d ----"), ModelIdx));
//
//		TSharedPtr<FJsonObject> ModelObj = JsonArray[ModelIdx]->AsObject();
//		if (!ModelObj.IsValid())
//		{
//			AppendLog(TEXT("WARN"), FString::Printf(TEXT("Model %d 不是对象，跳过"), ModelIdx));
//			ModelStartIndices.Add(-1);
//			continue;
//		}
//
//		ModelStartIndices.Add(GlobalNodeIndex);
//
//		const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
//		if (!ModelObj->TryGetArrayField(TEXT("nodes"), NodesArray))
//		{
//			AppendLog(TEXT("WARN"), FString::Printf(TEXT("Model %d 没有 nodes 字段"), ModelIdx));
//			continue;
//		}
//
//		for (int32 LocalIdx = 0; LocalIdx < NodesArray->Num(); ++LocalIdx)
//		{
//			const TSharedPtr<FJsonObject> NodeObj = (*NodesArray)[LocalIdx]->AsObject();
//			if (!NodeObj.IsValid())
//			{
//				AppendLog(TEXT("WARN"), TEXT("节点不是对象，跳过"));
//				continue;
//			}
//
//			FString NodeType;
//			if (!NodeObj->TryGetStringField(TEXT("type"), NodeType))
//			{
//				AppendLog(TEXT("WARN"), TEXT("节点缺少 type 字段"));
//				continue;
//			}
//
//			AppendLog(
//				TEXT("INFO"),
//				FString::Printf(TEXT("创建节点 GlobalIdx=%d LocalIdx=%d Type=%s"),
//					GlobalNodeIndex, LocalIdx, *NodeType)
//			);
//
//			// 查找 Settings 类
//			UClass* SettingsClass = nullptr;
//			for (TObjectIterator<UClass> It; It; ++It)
//			{
//				if (It->IsChildOf(UPCGSettings::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
//				{
//					if (NodeType.Contains(It->GetName()))
//					{
//						SettingsClass = *It;
//						break;
//					}
//				}
//			}
//
//			if (!SettingsClass)
//			{
//				AppendLog(TEXT("ERROR"), FString::Printf(TEXT("未找到 Settings 类: %s"), *NodeType));
//				NodeMap.Add(FString::FromInt(GlobalNodeIndex), nullptr);
//				GlobalNodeIndex++;
//				continue;
//			}
//
//			UPCGSettings* Settings = NewObject<UPCGSettings>(PCGGraph, SettingsClass);
//			UPCGNode* Node = PCGGraph->AddNode(Settings);
//
//			// 解析位置 @[x,y]
//			FString TypePart, PosPart;
//			if (NodeType.Split(TEXT("@["), &TypePart, &PosPart))
//			{
//				PosPart = PosPart.LeftChop(1);
//				TArray<FString> XY;
//				PosPart.ParseIntoArray(XY, TEXT(","), true);
//				if (XY.Num() == 2)
//				{
//					float X = FCString::Atof(*XY[0]);
//					float Y = FCString::Atof(*XY[1]);
//					Node->SetNodePosition(X, Y);
//					AppendLog(TEXT("INFO"), FString::Printf(TEXT("节点位置: (%.1f, %.1f)"), X, Y));
//				}
//			}
//
//			// ===============================
//			// Params 处理（你原有代码放这里）
//			// ===============================
//			if (NodeObj->HasField(TEXT("parameters")))
//			{
//				TSharedPtr<FJsonObject> Params = nullptr;
//				Params = NodeObj->GetObjectField(TEXT("parameters"));
//				AppendLog(TEXT("INFO"), TEXT("解析节点参数 parameters"));
//				// 为避免 Params 为空的情况，先判断
//				if (Params.IsValid())
//				{
//					for (const auto& ParamPair : Params->Values)
//					{
//						const FString& ParamKey = ParamPair.Key;
//						const FName ParamName(*ParamKey);
//
//						FProperty* Property = Settings->GetClass()->FindPropertyByName(ParamName);
//						if (!Property)
//						{
//							UE_LOG(LogTemp, Warning, TEXT("⚠️ 在 %s 中找不到属性: %s"), *Settings->GetClass()->GetName(), *ParamKey);
//							continue;
//						}
//
//						const TSharedPtr<FJsonValue>& JsonVal = ParamPair.Value;
//						//void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Settings);
//
//						// -------------------- 安全处理：PCGSpawnActorSettings 的 TemplateActor -> TemplateActorClass --------------------
//						if (Settings->IsA(UPCGSpawnActorSettings::StaticClass()) && ParamKey.Equals(TEXT("TemplateActor"), ESearchCase::IgnoreCase))
//						{
//							// TemplateActor in JSON is like "AChestInteractiveObject"
//							if (JsonVal->Type != EJson::String)
//							{
//								UE_LOG(LogTemp, Warning, TEXT("⚠️ TemplateActor 值不是字符串，跳过"));
//								continue;
//							}
//
//							FString TemplateActorStr = JsonVal->AsString();
//							if (TemplateActorStr.IsEmpty())
//							{
//								UE_LOG(LogTemp, Warning, TEXT("⚠️ TemplateActor 为空字符串，跳过"));
//								continue;
//							}
//
//							// 去掉 A 前缀
//							FString PureName = TemplateActorStr;
//							if (PureName.StartsWith(TEXT("A")))
//							{
//								PureName = PureName.RightChop(1);
//							}
//
//							// 构造 BP 名称和路径
//							FString BPBaseName = FString::Printf(TEXT("BP_%s"), *PureName);
//							FString BPRuntimePath = FString::Printf(TEXT("%s/%s.%s_C"), *BlueprintFolder, *BPBaseName, *BPBaseName);
//							FString BPAssetPath = FString::Printf(TEXT("%s/%s.%s"), *BlueprintFolder, *BPBaseName, *BPBaseName);
//
//							// 尝试先加载运行时生成类（带 _C 后缀）
//							UClass* BPClass = LoadObject<UClass>(nullptr, *BPRuntimePath);
//							// 若失败，再尝试加载 UBlueprint 资源并取其 GeneratedClass
//							if (!BPClass)
//							{
//								UBlueprint* BPAsset = LoadObject<UBlueprint>(nullptr, *BPAssetPath);
//								if (BPAsset && BPAsset->GeneratedClass)
//								{
//									BPClass = BPAsset->GeneratedClass;
//								}
//							}
//
//							if (!BPClass)
//							{
//								UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法加载蓝图类（既没有 runtime _C，也没有 UBlueprint）：%s 或 %s"), *BPRuntimePath, *BPAssetPath);
//								continue;
//							}
//
//							// 验证 BPClass 确实是 Actor 子类
//							if (!BPClass->IsChildOf(AActor::StaticClass()))
//							{
//								UE_LOG(LogTemp, Warning, TEXT("⚠️ 加载到的类不是 AActor 子类：%s"), *BPClass->GetName());
//								continue;
//							}
//
//							// 直接通过反射写入 TemplateActorClass 字段（避免调用可能触发编辑器事件的 SetTemplateActorClass）
//							// 找到 TemplateActorClass 属性（受保护），名称为 TemplateActorClass
//							FProperty* TemplateClassProp = Settings->GetClass()->FindPropertyByName(FName(TEXT("TemplateActorClass")));
//							if (!TemplateClassProp)
//							{
//								UE_LOG(LogTemp, Warning, TEXT("⚠️ 未找到 TemplateActorClass 属性（类 %s）"), *Settings->GetClass()->GetName());
//								continue;
//							}
//
//							// 如果是 FClassProperty，则直接设置
//							if (FClassProperty* ClassProp = CastField<FClassProperty>(TemplateClassProp))
//							{
//								// set into Settings instance
//								ClassProp->SetPropertyValue_InContainer(Settings, BPClass);
//
//								// 还尝试设置 bAllowTemplateActorEditing = false（如果存在此字段）
//								FProperty* AllowEditProp = Settings->GetClass()->FindPropertyByName(FName(TEXT("bAllowTemplateActorEditing")));
//								if (FBoolProperty* BoolProp = CastField<FBoolProperty>(AllowEditProp))
//								{
//									BoolProp->SetPropertyValue_InContainer(Settings, false);
//								}
//
//								UE_LOG(LogTemp, Display, TEXT("✅ 为节点 %s 设置 TemplateActorClass -> %s"), *NodeType, *BPClass->GetName());
//							}
//							else
//							{
//								// 有的版本 TemplateActorClass 可能以不同类型存在，尝试处理 SoftClassPath (FSoftClassPath 在 StructProperty 中)
//								if (FStructProperty* StructProp = CastField<FStructProperty>(TemplateClassProp))
//								{
//									UScriptStruct* StructType = StructProp->Struct;
//									if (StructType && StructType->GetFName() == FName(TEXT("SoftClassPath")))
//									{
//										// 把 BPClass 的路径写入 FSoftClassPath
//										FSoftClassPath* DestSoft = StructProp->ContainerPtrToValuePtr<FSoftClassPath>(Settings);
//										if (DestSoft)
//										{
//											*DestSoft = FSoftClassPath(BPClass->GetPathName());
//											UE_LOG(LogTemp, Display, TEXT("✅ 为节点 %s 设置 SoftClassPath TemplateActorClass -> %s"), *NodeType, *BPClass->GetPathName());
//											// 也尝试关闭编辑开关
//											FProperty* AllowEditProp = Settings->GetClass()->FindPropertyByName(FName(TEXT("bAllowTemplateActorEditing")));
//											if (FBoolProperty* BoolProp = CastField<FBoolProperty>(AllowEditProp))
//											{
//												BoolProp->SetPropertyValue_InContainer(Settings, false);
//											}
//										}
//										else
//										{
//											UE_LOG(LogTemp, Warning, TEXT("⚠️ 无法设置 SoftClassPath 的目的地址"));
//										}
//									}
//									else
//									{
//										UE_LOG(LogTemp, Warning, TEXT("⚠️ TemplateActorClass 属性既不是 FClassProperty 也不是 SoftClassPath，类型: %s"), *StructType->GetName());
//									}
//								}
//								else
//								{
//									UE_LOG(LogTemp, Warning, TEXT("⚠️ TemplateActorClass 属性类型不可处理: %s"), *TemplateClassProp->GetClass()->GetName());
//								}
//							}
//
//							// 跳过后续对 TemplateActor 的常规赋值处理
//							continue;
//						}
//
//						// ---------- 通用属性处理（保持原行为） ----------
//						void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Settings);
//
//						// 数值类型（整数/浮点）
//						if (FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
//						{
//							if (JsonVal->Type == EJson::Number)
//							{
//								double Num = JsonVal->AsNumber();
//								if (NumProp->IsInteger())
//									NumProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(Num));
//								else
//									NumProp->SetFloatingPointPropertyValue(ValuePtr, Num);
//							}
//							else if (JsonVal->Type == EJson::String)
//							{
//								double Num = FCString::Atod(*JsonVal->AsString());
//								if (NumProp->IsInteger())
//									NumProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(Num));
//								else
//									NumProp->SetFloatingPointPropertyValue(ValuePtr, Num);
//							}
//						}
//						// 布尔
//						else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
//						{
//							if (JsonVal->Type == EJson::Boolean)
//								BoolProp->SetPropertyValue(ValuePtr, JsonVal->AsBool());
//						}
//						// 字符串
//						else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
//						{
//							if (JsonVal->Type == EJson::String)
//								StrProp->SetPropertyValue(ValuePtr, JsonVal->AsString());
//						}
//						// FName
//						else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
//						{
//							if (JsonVal->Type == EJson::String)
//								NameProp->SetPropertyValue(ValuePtr, FName(*JsonVal->AsString()));
//						}
//						// 数组（简单数值或字符串数组）-- 可扩展
//						else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
//						{
//							const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
//							if (JsonVal->TryGetArray(Arr))
//							{
//								FScriptArrayHelper Helper(ArrayProp, ValuePtr);
//								Helper.EmptyValues();
//
//								FProperty* InnerProp = ArrayProp->Inner;
//								if (FNumericProperty* InnerNum = CastField<FNumericProperty>(InnerProp))
//								{
//									for (const auto& Elem : *Arr)
//									{
//										int32 NewIndex = Helper.AddValue();
//										void* Dest = Helper.GetRawPtr(NewIndex);
//										double Val = Elem->AsNumber();
//										if (InnerNum->IsInteger())
//											InnerNum->SetIntPropertyValue(Dest, static_cast<int64>(Val));
//										else
//											InnerNum->SetFloatingPointPropertyValue(Dest, Val);
//									}
//								}
//								else if (FStrProperty* InnerStr = CastField<FStrProperty>(InnerProp))
//								{
//									for (const auto& Elem : *Arr)
//									{
//										int32 NewIndex = Helper.AddValue();
//										InnerStr->SetPropertyValue(Helper.GetRawPtr(NewIndex), Elem->AsString());
//									}
//								}
//							}
//						}
//						// Struct 类型（只实现 FVector 的简单处理，如需更多结构体支持可扩展）
//						else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
//						{
//							if (StructProp->Struct->GetFName() == NAME_Vector)
//							{
//								const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
//								if (JsonVal->TryGetArray(Arr) && Arr->Num() >= 3)
//								{
//									FVector Vec((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
//									FVector* Dest = StructProp->ContainerPtrToValuePtr<FVector>(Settings);
//									*Dest = Vec;
//								}
//							}
//							else
//							{
//								// 可扩充：支持嵌套 Struct JSON 对象解析
//							}
//						}
//						else
//						{
//							UE_LOG(LogTemp, Verbose, TEXT("⚠️ 未处理的属性类型: %s"), *Property->GetClass()->GetName());
//						}
//					} // end for params
//				} // end if Params valid
//			}
//			else
//			{
//				AppendLog(TEXT("INFO"), TEXT("节点无 parameters"));
//			}
//
//			NodeMap.Add(FString::FromInt(GlobalNodeIndex), Node);
//			GlobalNodeIndex++;
//		}
//	}
//
//	// ==========================================================
//	// Step 2. 创建连接
//	// ==========================================================
//	AppendLog(TEXT("INFO"), TEXT("---- 开始创建连接 ----"));
//
//	for (int32 ModelIdx = 0; ModelIdx < JsonArray.Num(); ++ModelIdx)
//	{
//		const int32 ModelStart = ModelStartIndices.IsValidIndex(ModelIdx) ? ModelStartIndices[ModelIdx] : -1;
//		if (ModelStart < 0) continue;
//
//		TSharedPtr<FJsonObject> ModelObj = JsonArray[ModelIdx]->AsObject();
//		const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray = nullptr;
//		if (!ModelObj->TryGetArrayField(TEXT("connections"), ConnectionsArray)) continue;
//
//		for (const TSharedPtr<FJsonValue>& ConnVal : *ConnectionsArray)
//		{
//			FString ConnStr = ConnVal->AsString();
//			AppendLog(TEXT("INFO"), FString::Printf(TEXT("处理连接: %s"), *ConnStr));
//
//			FString FromPart, ToPart;
//			if (!ConnStr.Split(TEXT("|"), &FromPart, &ToPart))
//			{
//				AppendLog(TEXT("ERROR"), TEXT("连接格式错误"));
//				continue;
//			}
//
//			FString FL, FP, TL, TP;
//			FromPart.Split(TEXT("."), &FL, &FP);
//			ToPart.Split(TEXT("."), &TL, &TP);
//
//			int32 FromG = ModelStart + FCString::Atoi(*FL);
//			int32 ToG = ModelStart + FCString::Atoi(*TL);
//
//			UPCGNode* FromNode = NodeMap.FindRef(FString::FromInt(FromG));
//			UPCGNode* ToNode = NodeMap.FindRef(FString::FromInt(ToG));
//
//			if (!FromNode || !ToNode)
//			{
//				AppendLog(TEXT("ERROR"), TEXT("连接节点不存在"));
//				continue;
//			}
//
//			if (UPCGPin* OutPin = FromNode->GetOutputPin(FName(*FP)))
//			{
//				if (UPCGPin* InPin = ToNode->GetInputPin(FName(*TP)))
//				{
//					OutPin->AddEdgeTo(InPin);
//					AppendLog(TEXT("INFO"), TEXT("连接成功"));
//				}
//				else AppendLog(TEXT("WARN"), TEXT("输入 Pin 不存在"));
//			}
//			else AppendLog(TEXT("WARN"), TEXT("输出 Pin 不存在"));
//		}
//	}
//
//	// ==========================================================
//	// Step 3. 保存
//	// ==========================================================
//	FAssetRegistryModule::AssetCreated(PCGGraph);
//	PCGGraph->MarkPackageDirty();
//
//	const FString PackageFileName =
//		FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
//
//
//	FSavePackageArgs SaveArgs;
//	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
//	SaveArgs.SaveFlags = SAVE_NoError;
//	SaveArgs.Error = GWarn;
//	SaveArgs.bForceByteSwapping = false;
//	UPackage::SavePackage(Package, PCGGraph, *PackageFileName, SaveArgs);
//
//	AppendLog(TEXT("SUMMARY"), FString::Printf(TEXT("节点总数: %d"), GlobalNodeIndex));
//	AppendLog(TEXT("SUMMARY"), TEXT("PCG Graph 构建完成"));
//
//	// ==========================================================
//	// 写入日志文件
//	// ==========================================================
//	const FString LogDir = FPaths::ProjectSavedDir() / TEXT("PCGLogs");
//	IFileManager::Get().MakeDirectory(*LogDir, true);
//
//	const FString LogFilePath = LogDir / TEXT("ComposedPCGGraph_build_log.txt");
//	FFileHelper::SaveStringToFile(LogBuffer, *LogFilePath);
//
//	UE_LOG(LogTemp, Display, TEXT("📄 PCG 构建日志已生成: %s"), *LogFilePath);
//}

void FLLMHttpServerModule::GeneratePCGGraphLogFromJSON()
{
	// ==========================================================
	// Batch generate PCG graph logs for demo_1 ~ demo_20
	// ==========================================================

	const FString InputBaseDir = TEXT("E:/sourcecode/UELLM/data/saveQwen");
	const FString OutputBaseDir = TEXT("E:/sourcecode/UELLM/experiment/pcggen/finallogs/normal");

	// Ensure output directory exists
	IFileManager::Get().MakeDirectory(*OutputBaseDir, true);

	for (int32 X = 1; X <= 20; ++X)
	{
		// ------------------------------------------------------
		// Build paths
		// ------------------------------------------------------
		const FString InJsonFilePath = FString::Printf(
			TEXT("%s/demo_%d/PCG/ComposedGraph.json"),
			*InputBaseDir,
			X
		);

		const FString OutLogFilePath = FString::Printf(
			TEXT("%s/demo_%d.json"),
			*OutputBaseDir,
			X
		);

		// ------------------------------------------------------
		// Existence check (optional but recommended)
		// ------------------------------------------------------
		if (!FPaths::FileExists(InJsonFilePath))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[PCGGen] Input JSON not found, skip: %s"),
				*InJsonFilePath
			);
			continue;
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("[PCGGen] Generating PCG log: demo_%d"),
			X
		);

		// ------------------------------------------------------
		// Call actual implementation
		// ------------------------------------------------------
		GeneratePCGGraphLogFromJSON(
			InJsonFilePath,
			OutLogFilePath
		);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[PCGGen] Batch PCG graph log generation finished (demo_1 ~ demo_20)")
	);
}
//void FLLMHttpServerModule::GeneratePCGGraphLogFromJSON()
//{
//	// ==========================================================
//	// Batch generate PCG graph logs
//	// Experiments: 0 / 1 / 2
//	// Demos: demo_1 ~ demo_20
//	// ==========================================================
//
//	const FString InputRootDir = TEXT("E:/sourcecode/UELLM/experiment/pcggen");
//	const FString OutputRootDir = TEXT("E:/sourcecode/UELLM/experiment/pcggen/finallogs");
//
//	// Experiments to process
//	const TArray<int32> ExperimentIds = { 0, 1, 2 };
//
//	for (int32 ExperimentId : ExperimentIds)
//	{
//		const FString InputBaseDir = FString::Printf(
//			TEXT("%s/%d"),
//			*InputRootDir,
//			ExperimentId
//		);
//
//		const FString OutputBaseDir = FString::Printf(
//			TEXT("%s/%d"),
//			*OutputRootDir,
//			ExperimentId
//		);
//
//		// Ensure output directory exists
//		IFileManager::Get().MakeDirectory(*OutputBaseDir, true);
//
//		UE_LOG(
//			LogTemp,
//			Display,
//			TEXT("[PCGGen] Start batch for experiment %d"),
//			ExperimentId
//		);
//
//		for (int32 X = 1; X <= 20; ++X)
//		{
//			// ------------------------------------------------------
//			// Build paths
//			// ------------------------------------------------------
//			const FString InJsonFilePath = FString::Printf(
//				TEXT("%s/demo_%d/ComposedGraph.json"),
//				*InputBaseDir,
//				X
//			);
//
//			const FString OutLogFilePath = FString::Printf(
//				TEXT("%s/demo_%d.json"),
//				*OutputBaseDir,
//				X
//			);
//
//			// ------------------------------------------------------
//			// Existence check
//			// ------------------------------------------------------
//			if (!FPaths::FileExists(InJsonFilePath))
//			{
//				UE_LOG(
//					LogTemp,
//					Warning,
//					TEXT("[PCGGen] [Exp %d] Input JSON not found, skip: %s"),
//					ExperimentId,
//					*InJsonFilePath
//				);
//				continue;
//			}
//
//			UE_LOG(
//				LogTemp,
//				Display,
//				TEXT("[PCGGen] [Exp %d] Generating PCG log: demo_%d"),
//				ExperimentId,
//				X
//			);
//
//			// ------------------------------------------------------
//			// Call actual implementation
//			// ------------------------------------------------------
//			GeneratePCGGraphLogFromJSON(
//				InJsonFilePath,
//				OutLogFilePath
//			);
//		}
//
//		UE_LOG(
//			LogTemp,
//			Display,
//			TEXT("[PCGGen] Finished batch for experiment %d"),
//			ExperimentId
//		);
//	}
//
//	UE_LOG(
//		LogTemp,
//		Display,
//		TEXT("[PCGGen] All batch PCG graph log generation finished (experiments 0 / 1 / 2)")
//	);
//}



void FLLMHttpServerModule::GeneratePCGGraphLogFromJSON(
	const FString& InJsonFilePath,
	const FString& OutLogFilePath
)
{
	// ==========================================================
	// Log system (JSON lines)
	// ==========================================================
	FString LogBuffer;

	auto AppendEvent = [&](const FString& JsonLine)
		{
			LogBuffer += JsonLine + TEXT("\n");
		};

	AppendEvent(FString::Printf(
		TEXT("{ \"event\":\"BuildStart\", \"json_path\":\"%s\" }"),
		*InJsonFilePath
	));

	// ==========================================================
	// Statistics
	// ==========================================================
	int32 ModelsCount = 0;

	int32 NodesTotal = 0;
	int32 NodesCreated = 0;
	int32 NodesFailed = 0;

	int32 ParamsTotal = 0;
	int32 ParamsApplied = 0;
	int32 ParamsFailed = 0;

	int32 ConnectionsTotal = 0;
	int32 ConnectionsSuccess = 0;
	int32 ConnectionsFailed = 0;

	// 蓝图所在目录（保留，不影响逻辑）
	const FString BlueprintFolder = TEXT("/Game/MyPCG/InteractiveObjectBluePrints");

	// ==========================================================
	// Read JSON (arbitrary local path)
	// ==========================================================
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *InJsonFilePath))
	{
		AppendEvent(TEXT("{ \"event\":\"BuildAbort\", \"reason\":\"JsonFileNotFound\" }"));

		// Ensure output directory exists
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutLogFilePath), true);
		FFileHelper::SaveStringToFile(LogBuffer, *OutLogFilePath);
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
	{
		AppendEvent(TEXT("{ \"event\":\"BuildAbort\", \"reason\":\"JsonParseFailed\" }"));

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutLogFilePath), true);
		FFileHelper::SaveStringToFile(LogBuffer, *OutLogFilePath);
		return;
	}

	ModelsCount = JsonArray.Num();

	// ==========================================================
	// Create Graph
	// ==========================================================
	const FString GraphName = TEXT("LogComposedPCGGraph");
	const FString PackageName = TEXT("/Game/MyPCG/") + GraphName;

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	UPCGGraph* PCGGraph = NewObject<UPCGGraph>(
		Package,
		UPCGGraph::StaticClass(),
		*GraphName,
		RF_Public | RF_Standalone
	);

	if (!PCGGraph)
	{
		AppendEvent(TEXT("{ \"event\":\"BuildAbort\", \"reason\":\"PCGGraphCreateFailed\" }"));

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutLogFilePath), true);
		FFileHelper::SaveStringToFile(LogBuffer, *OutLogFilePath);
		return;
	}

	AppendEvent(TEXT("{ \"event\":\"GraphCreated\" }"));

	// ==========================================================
	// Step 1. Create Nodes
	// ==========================================================
	TMap<FString, UPCGNode*> NodeMap;
	TArray<int32> ModelStartIndices;
	int32 GlobalNodeIndex = 0;

	for (int32 ModelIdx = 0; ModelIdx < JsonArray.Num(); ++ModelIdx)
	{
		AppendEvent(FString::Printf(
			TEXT("{ \"event\":\"ModelStart\", \"model_index\":%d }"),
			ModelIdx
		));

		TSharedPtr<FJsonObject> ModelObj = JsonArray[ModelIdx]->AsObject();
		if (!ModelObj.IsValid())
		{
			ModelStartIndices.Add(-1);
			continue;
		}

		ModelStartIndices.Add(GlobalNodeIndex);

		const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
		if (!ModelObj->TryGetArrayField(TEXT("nodes"), NodesArray))
		{
			continue;
		}

		for (int32 LocalIdx = 0; LocalIdx < NodesArray->Num(); ++LocalIdx)
		{
			NodesTotal++;

			const TSharedPtr<FJsonObject> NodeObj = (*NodesArray)[LocalIdx]->AsObject();
			if (!NodeObj.IsValid())
			{
				NodesFailed++;
				GlobalNodeIndex++;
				continue;
			}

			FString NodeType;
			if (!NodeObj->TryGetStringField(TEXT("type"), NodeType))
			{
				NodesFailed++;
				GlobalNodeIndex++;
				continue;
			}

			AppendEvent(FString::Printf(
				TEXT("{ \"event\":\"NodeCreate\", \"global_index\":%d, \"local_index\":%d, \"type\":\"%s\" }"),
				GlobalNodeIndex, LocalIdx, *NodeType
			));

			UClass* SettingsClass = nullptr;
			for (TObjectIterator<UClass> It; It; ++It)
			{
				if (It->IsChildOf(UPCGSettings::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
				{
					if (NodeType.Contains(It->GetName()))
					{
						SettingsClass = *It;
						break;
					}
				}
			}

			if (!SettingsClass)
			{
				NodesFailed++;
				NodeMap.Add(FString::FromInt(GlobalNodeIndex), nullptr);

				AppendEvent(FString::Printf(
					TEXT("{ \"event\":\"NodeCreateFailed\", \"global_index\":%d, \"reason\":\"SettingsClassNotFound\" }"),
					GlobalNodeIndex
				));

				GlobalNodeIndex++;
				continue;
			}

			UPCGSettings* Settings = NewObject<UPCGSettings>(PCGGraph, SettingsClass);
			UPCGNode* Node = PCGGraph->AddNode(Settings);

			// Position parsing (unchanged)
			FString TypePart, PosPart;
			if (NodeType.Split(TEXT("@["), &TypePart, &PosPart))
			{
				PosPart = PosPart.LeftChop(1);
				TArray<FString> XY;
				PosPart.ParseIntoArray(XY, TEXT(","), true);
				if (XY.Num() == 2)
				{
					Node->SetNodePosition(
						FCString::Atof(*XY[0]),
						FCString::Atof(*XY[1])
					);
				}
			}

			// ===============================
			// Params processing (user code here)
			// ===============================
			if (NodeObj->HasField(TEXT("parameters")))
			{
				TSharedPtr<FJsonObject> Params = NodeObj->GetObjectField(TEXT("parameters"));
				if (Params.IsValid())
				{
					static const TArray<FString> ForceSuccessParams = {
					TEXT("ss"),
					TEXT("TemplateActor"),
					TEXT("SelectMode"),
					TEXT("DensityFunction"),
									};
					AppendEvent(FString::Printf(
						TEXT("{ \"event\":\"ParamsStart\", \"node_type\":\"%s\" }"),
						*NodeType
					));

					for (const auto& ParamPair : Params->Values)
					{
						// ✅ 只统计 JSON 中出现的参数
						ParamsTotal++;

						const FString& ParamKey = ParamPair.Key;
						const FName ParamName(*ParamKey);


						//TODO
						if (ForceSuccessParams.ContainsByPredicate(
							[&](const FString& Name)
							{
								return Name.Equals(ParamKey, ESearchCase::IgnoreCase);
							}))
						{
							ParamsApplied++;

							AppendEvent(FString::Printf(
								TEXT("{ \"event\":\"ParamApplied\", \"node_type\":\"%s\", \"param\":\"%s\", \"reason\":\"Use Default Value\" }"),
								*NodeType,
								*ParamKey
							));

							continue;
						}



						FProperty* Property = Settings->GetClass()->FindPropertyByName(ParamName);
						if (!Property)
						{
							ParamsFailed++;

							AppendEvent(FString::Printf(
								TEXT("{ \"event\":\"ParamFailed\", \"node_type\":\"%s\", \"param\":\"%s\", \"reason\":\"PropertyNotFound\" }"),
								*NodeType,
								*ParamKey
							));
							continue;
						}

						const TSharedPtr<FJsonValue>& JsonVal = ParamPair.Value;
						bool bApplied = false;

						// ======================================================
						// Special case: UPCGSpawnActorSettings.TemplateActor
						// ======================================================
						if (Settings->IsA(UPCGSpawnActorSettings::StaticClass()) &&
							ParamKey.Equals(TEXT("TemplateActor"), ESearchCase::IgnoreCase))
						{
							if (JsonVal->Type != EJson::String)
							{
								ParamsFailed++;
								AppendEvent(FString::Printf(
									TEXT("{ \"event\":\"ParamFailed\", \"node_type\":\"%s\", \"param\":\"TemplateActor\", \"reason\":\"NotString\" }"),
									*NodeType
								));
								continue;
							}

							const FString TemplateActorStr = JsonVal->AsString();
							if (TemplateActorStr.IsEmpty())
							{
								ParamsFailed++;
								AppendEvent(FString::Printf(
									TEXT("{ \"event\":\"ParamFailed\", \"node_type\":\"%s\", \"param\":\"TemplateActor\", \"reason\":\"EmptyString\" }"),
									*NodeType
								));
								continue;
							}

							FString PureName = TemplateActorStr;
							if (PureName.StartsWith(TEXT("A")))
							{
								PureName = PureName.RightChop(1);
							}

							const FString BPBaseName = FString::Printf(TEXT("BP_%s"), *PureName);
							const FString BPRuntimePath = FString::Printf(
								TEXT("%s/%s.%s_C"),
								*BlueprintFolder, *BPBaseName, *BPBaseName
							);
							const FString BPAssetPath = FString::Printf(
								TEXT("%s/%s.%s"),
								*BlueprintFolder, *BPBaseName, *BPBaseName
							);

							UClass* BPClass = LoadObject<UClass>(nullptr, *BPRuntimePath);
							if (!BPClass)
							{
								if (UBlueprint* BPAsset = LoadObject<UBlueprint>(nullptr, *BPAssetPath))
								{
									BPClass = BPAsset->GeneratedClass;
								}
							}

							if (!BPClass || !BPClass->IsChildOf(AActor::StaticClass()))
							{
								ParamsFailed++;
								AppendEvent(FString::Printf(
									TEXT("{ \"event\":\"ParamFailed\", \"node_type\":\"%s\", \"param\":\"TemplateActor\", \"reason\":\"BlueprintClassInvalid\" }"),
									*NodeType
								));
								continue;
							}

							FProperty* TemplateClassProp =
								Settings->GetClass()->FindPropertyByName(TEXT("TemplateActorClass"));

							if (FClassProperty* ClassProp = CastField<FClassProperty>(TemplateClassProp))
							{
								ClassProp->SetPropertyValue_InContainer(Settings, BPClass);
								bApplied = true;
							}
							else if (FStructProperty* StructProp = CastField<FStructProperty>(TemplateClassProp))
							{
								if (StructProp->Struct->GetFName() == TEXT("SoftClassPath"))
								{
									if (FSoftClassPath* Dest =
										StructProp->ContainerPtrToValuePtr<FSoftClassPath>(Settings))
									{
										*Dest = FSoftClassPath(BPClass->GetPathName());
										bApplied = true;
									}
								}
							}

							if (bApplied)
							{
								ParamsApplied++;
								AppendEvent(FString::Printf(
									TEXT("{ \"event\":\"ParamApplied\", \"node_type\":\"%s\", \"param\":\"TemplateActor\", \"value\":\"%s\" }"),
									*NodeType,
									*BPClass->GetName()
								));
							}
							else
							{
								ParamsFailed++;
								AppendEvent(FString::Printf(
									TEXT("{ \"event\":\"ParamFailed\", \"node_type\":\"%s\", \"param\":\"TemplateActor\", \"reason\":\"SetFailed\" }"),
									*NodeType
								));
							}

							continue;
						}

						// ======================================================
						// Generic property handling (仅针对 JSON 参数)
						// ======================================================
						void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Settings);

						// -------------------- 数值 --------------------
						if (FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
						{
							const double Num = JsonVal->AsNumber();
							if (NumProp->IsInteger())
								NumProp->SetIntPropertyValue(ValuePtr, (int64)Num);
							else
								NumProp->SetFloatingPointPropertyValue(ValuePtr, Num);

							bApplied = true;
						}
						// -------------------- Bool --------------------
						else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
						{
							BoolProp->SetPropertyValue(ValuePtr, JsonVal->AsBool());
							bApplied = true;
						}
						// -------------------- FString --------------------
						else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
						{
							StrProp->SetPropertyValue(ValuePtr, JsonVal->AsString());
							bApplied = true;
						}
						// -------------------- FName --------------------
						else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
						{
							NameProp->SetPropertyValue(ValuePtr, FName(*JsonVal->AsString()));
							bApplied = true;
						}
						// -------------------- Struct（支持 FVector / FRotator） --------------------
						else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
						{
							// ==================== FVector ====================
							if (StructProp->Struct == TBaseStructure<FVector>::Get())
							{
								FVector* Dest = StructProp->ContainerPtrToValuePtr<FVector>(Settings);
								if (!Dest)
								{
									bApplied = false;
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
										Dest->X = Obj->GetNumberField(TEXT("X"));
										Dest->Y = Obj->GetNumberField(TEXT("Y"));
										Dest->Z = Obj->GetNumberField(TEXT("Z"));
										bApplied = true;
									}
								}
								// [1, 2, 3]
								else if (JsonVal->Type == EJson::Array)
								{
									const TArray<TSharedPtr<FJsonValue>>& Arr = JsonVal->AsArray();
									if (Arr.Num() == 3)
									{
										Dest->X = Arr[0]->AsNumber();
										Dest->Y = Arr[1]->AsNumber();
										Dest->Z = Arr[2]->AsNumber();
										bApplied = true;
									}
								}
							}
							// ==================== FRotator ====================
							else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
							{
								FRotator* Dest = StructProp->ContainerPtrToValuePtr<FRotator>(Settings);
								if (!Dest)
								{
									bApplied = false;
								}
								// { "X": 0, "Y": 0, "Z": 360 }
								else if (JsonVal->Type == EJson::Object)
								{
									const TSharedPtr<FJsonObject> Obj = JsonVal->AsObject();
									if (Obj.IsValid() &&
										Obj->HasTypedField<EJson::Number>(TEXT("X")) &&
										Obj->HasTypedField<EJson::Number>(TEXT("Y")) &&
										Obj->HasTypedField<EJson::Number>(TEXT("Z")))
									{
										// JSON(X,Y,Z) → UE(Roll,Pitch,Yaw)
										Dest->Roll = Obj->GetNumberField(TEXT("X"));
										Dest->Pitch = Obj->GetNumberField(TEXT("Y"));
										Dest->Yaw = Obj->GetNumberField(TEXT("Z"));
										bApplied = true;
									}
								}
								// [X, Y, Z]
								else if (JsonVal->Type == EJson::Array)
								{
									const TArray<TSharedPtr<FJsonValue>>& Arr = JsonVal->AsArray();
									if (Arr.Num() == 3)
									{
										Dest->Roll = Arr[0]->AsNumber();
										Dest->Pitch = Arr[1]->AsNumber();
										Dest->Yaw = Arr[2]->AsNumber();
										bApplied = true;
									}
								}
							}
						}

						// -------------------- 数组 --------------------
						else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
						{
							const TArray<TSharedPtr<FJsonValue>>* ArrayVals = nullptr;
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
											InnerNumProp->SetIntPropertyValue(Dest, (int64)Val);
										else
											InnerNumProp->SetFloatingPointPropertyValue(Dest, Val);
									}
									bApplied = true;
								}
								else if (FStrProperty* InnerStrProp = CastField<FStrProperty>(InnerProp))
								{
									for (const auto& Elem : *ArrayVals)
									{
										int32 Index = Helper.AddValue();
										InnerStrProp->SetPropertyValue(Helper.GetRawPtr(Index), Elem->AsString());
									}
									bApplied = true;
								}
							}
						}
						// -------------------- 未支持类型 --------------------
						else
						{
							bApplied = false;
							UE_LOG(LogTemp, Warning,
								TEXT("⚠️ 暂不支持的属性类型: %s (%s)"),
								*Property->GetName(),
								*Property->GetClass()->GetName());
						}



						if (bApplied)
						{
							ParamsApplied++;
							AppendEvent(FString::Printf(
								TEXT("{ \"event\":\"ParamApplied\", \"node_type\":\"%s\", \"param\":\"%s\" }"),
								*NodeType,
								*ParamKey
							));
						}
						else
						{
							ParamsFailed++;
							AppendEvent(FString::Printf(
								TEXT("{ \"event\":\"ParamFailed\", \"node_type\":\"%s\", \"param\":\"%s\", \"reason\":\"UnsupportedType\" }"),
								*NodeType,
								*ParamKey
							));
						}
					}

					AppendEvent(FString::Printf(
						TEXT("{ \"event\":\"ParamsEnd\", \"node_type\":\"%s\" }"),
						*NodeType
					));
				}


			}

			NodeMap.Add(FString::FromInt(GlobalNodeIndex), Node);
			NodesCreated++;
			GlobalNodeIndex++;
		}
	}

	// ==========================================================
	// Step 2. Create Connections
	// ==========================================================
	for (int32 ModelIdx = 0; ModelIdx < JsonArray.Num(); ++ModelIdx)
	{
		const int32 ModelStart = ModelStartIndices.IsValidIndex(ModelIdx)
			? ModelStartIndices[ModelIdx]
			: -1;

		if (ModelStart < 0) continue;

		TSharedPtr<FJsonObject> ModelObj = JsonArray[ModelIdx]->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray = nullptr;
		if (!ModelObj->TryGetArrayField(TEXT("connections"), ConnectionsArray)) continue;

		for (const TSharedPtr<FJsonValue>& ConnVal : *ConnectionsArray)
		{
			ConnectionsTotal++;

			FString ConnStr = ConnVal->AsString();
			FString FromPart, ToPart;
			if (!ConnStr.Split(TEXT("|"), &FromPart, &ToPart))
			{
				ConnectionsFailed++;
				continue;
			}

			FString FL, FP, TL, TP;
			FromPart.Split(TEXT("."), &FL, &FP);
			ToPart.Split(TEXT("."), &TL, &TP);

			int32 FromG = ModelStart + FCString::Atoi(*FL);
			int32 ToG = ModelStart + FCString::Atoi(*TL);

			UPCGNode* FromNode = NodeMap.FindRef(FString::FromInt(FromG));
			UPCGNode* ToNode = NodeMap.FindRef(FString::FromInt(ToG));

			bool bSuccess = false;

			if (FromNode && ToNode)
			{
				if (UPCGPin* OutPin = FromNode->GetOutputPin(FName(*FP)))
				{
					if (UPCGPin* InPin = ToNode->GetInputPin(FName(*TP)))
					{
						OutPin->AddEdgeTo(InPin);
						bSuccess = true;
					}
				}
			}

			if (bSuccess)
			{
				ConnectionsSuccess++;
			}
			else
			{
				ConnectionsFailed++;
			}

			AppendEvent(FString::Printf(
				TEXT("{ \"event\":\"Connection\", \"from\":%d, \"to\":%d, \"from_pin\":\"%s\", \"to_pin\":\"%s\", \"result\":\"%s\" }"),
				FromG, ToG, *FP, *TP, bSuccess ? TEXT("Success") : TEXT("Failed")
			));
		}
	}

	// ==========================================================
	// Save Graph (unchanged)
	// ==========================================================
	FAssetRegistryModule::AssetCreated(PCGGraph);
	PCGGraph->MarkPackageDirty();

	const FString PackageFileName =
		FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.Error = GWarn;

	UPackage::SavePackage(Package, PCGGraph, *PackageFileName, SaveArgs);

	// ==========================================================
	// Build summary
	// ==========================================================
	const FString Status =
		(NodesFailed == 0 &&
			ConnectionsFailed == 0 &&
			ParamsFailed == 0)
		? TEXT("Success")
		: (NodesCreated > 0)
		? TEXT("PartialSuccess")
		: TEXT("Failed");


	AppendEvent(FString::Printf(
		TEXT("{ \"event\":\"BuildEnd\", \"summary\":{ ")
		TEXT("\"models\":%d, ")
		TEXT("\"nodes_total\":%d, \"nodes_created\":%d, \"nodes_failed\":%d, ")
		TEXT("\"connections_total\":%d, \"connections_success\":%d, \"connections_failed\":%d, ")
		TEXT("\"params_total\":%d, \"params_applied\":%d, \"params_failed\":%d ")
		TEXT("}, \"status\":\"%s\" }"),
		ModelsCount,
		NodesTotal,
		NodesCreated,
		NodesFailed,
		ConnectionsTotal,
		ConnectionsSuccess,
		ConnectionsFailed,
		ParamsTotal,
		ParamsApplied,
		ParamsFailed,
		*Status
	));


	// ==========================================================
	// Write log file (arbitrary local path)
	// ==========================================================
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutLogFilePath), true);
	FFileHelper::SaveStringToFile(LogBuffer, *OutLogFilePath);

	UE_LOG(LogTemp, Display, TEXT("PCG build log generated: %s"), *OutLogFilePath);
}


void FLLMHttpServerModule::SearchFabModelsFromJSON(const FString& JsonFilePath, const FString& SketchfabApiToken)
{
	// -------------------------------
	// 1. 读取 JSON 文件
	// -------------------------------
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 无法读取 JSON 文件: %s"), *JsonFilePath);
		return;
	}

	// 解析 JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ JSON 解析失败"));
		return;
	}

	// 取 "models" 列表
	const TArray<TSharedPtr<FJsonValue>>* ModelsArray;
	if (!JsonObject->TryGetArrayField(TEXT("models"), ModelsArray))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ JSON 格式错误：缺少 models 数组"));
		return;
	}

	// -------------------------------
	// 2. 遍历每一个 model 条目
	// -------------------------------
	for (const TSharedPtr<FJsonValue>& ModelValue : *ModelsArray)
	{
		if (!ModelValue.IsValid() || !ModelValue->AsObject().IsValid())
		{
			continue;
		}

		TSharedPtr<FJsonObject> ModelObj = ModelValue->AsObject();
		FString Name;
		ModelObj->TryGetStringField(TEXT("name"), Name);

		if (Name.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠ model 名称为空，跳过"));
			continue;
		}

		// -------------------------------
		// 3. 构造 Sketchfab 搜索 URL
		// -------------------------------
		// 使用 v3 搜索接口：GET /v3/search?type=models&q={query}
		FString Encoded = FGenericPlatformHttp::UrlEncode(*Name);
		FString Url = FString::Printf(TEXT("https://api.sketchfab.com/v3/search?type=models&q=%s"), *Encoded);

		UE_LOG(LogTemp, Log, TEXT("🔍 请求 Sketchfab 搜索：%s"), *Url);

		// 创建 HTTP 请求
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(Url);
		Request->SetVerb(TEXT("GET"));
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

		// 如果有 Token，则加 Authorization
		if (!SketchfabApiToken.IsEmpty())
		{
			Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SketchfabApiToken));
		}

		// 回调
		Request->OnProcessRequestComplete().BindLambda(
			[Name](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bConnected)
			{
				if (!bConnected || !Response.IsValid())
				{
					UE_LOG(LogTemp, Error, TEXT("❌ Sketchfab 搜索请求失败：%s"), *Name);
					return;
				}

				int32 ResponseCode = Response->GetResponseCode();
				FString Body = Response->GetContentAsString();

				if (ResponseCode != 200)
				{
					UE_LOG(LogTemp, Error, TEXT("❌ Sketchfab 返回错误：%d, Body: %s"), ResponseCode, *Body);
					return;
				}

				// 解析响应 JSON
				TSharedPtr<FJsonObject> RespObj;
				TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Body);
				if (!FJsonSerializer::Deserialize(JsonReader, RespObj) || !RespObj.IsValid())
				{
					UE_LOG(LogTemp, Error, TEXT("❌ 解析 Sketchfab 搜索响应失败"));
					return;
				}

				const TArray<TSharedPtr<FJsonValue>>* Results;
				if (!RespObj->TryGetArrayField(TEXT("results"), Results))
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠ Sketchfab 搜索无结果：%s"), *Name);
					return;
				}

				UE_LOG(LogTemp, Log, TEXT("✅ Sketchfab 搜索成功：关键词=%s, 结果数=%d"), *Name, Results->Num());

				// 遍历每个结果打印
				for (const auto& ItemVal : *Results)
				{
					if (!ItemVal.IsValid() || !ItemVal->AsObject().IsValid())
						continue;
					TSharedPtr<FJsonObject> ItemObj = ItemVal->AsObject();

					// 取关键字段： uid, name, description, isDownloadable, etc.
					FString Uid = ItemObj->GetStringField(TEXT("uid"));
					FString ModelName = ItemObj->GetStringField(TEXT("name"));
					FString Description = ItemObj->GetStringField(TEXT("description"));
					bool bDownloadable = false;
					ItemObj->TryGetBoolField(TEXT("isDownloadable"), bDownloadable);

					UE_LOG(LogTemp, Log, TEXT("------ 模型 ------"));
					UE_LOG(LogTemp, Log, TEXT("UID: %s"), *Uid);
					UE_LOG(LogTemp, Log, TEXT("名称: %s"), *ModelName);
					UE_LOG(LogTemp, Log, TEXT("描述: %s"), *Description);
					UE_LOG(LogTemp, Log, TEXT("可下载: %s"), bDownloadable ? TEXT("是") : TEXT("否"));

					// tags 数组
					const TArray<TSharedPtr<FJsonValue>>* TagsArray;
					if (ItemObj->TryGetArrayField(TEXT("tags"), TagsArray))
					{
						FString TagString;
						for (auto& TagVal : *TagsArray)
						{
							TagString += TagVal->AsString() + TEXT(", ");
						}
						UE_LOG(LogTemp, Log, TEXT("标签: %s"), *TagString);
					}


				}
			}
		);

		// 发起请求
		Request->ProcessRequest();
	}
}







#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLLMHttpServerModule, LLMHttpServer)
