// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "HttpRouteHandle.h"
#include "Templates/SharedPointer.h"

class IHttpRouter;
class SLLMEditorWindow;

class FLLMHttpServerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** 打开 Editor 窗口 */
	void OpenLLMWindow();

private:

	/** 注册菜单 */
	void RegisterMenus();

	/** HTTP Server */
	void StartHttpServer();
	void StopHttpServer();

	/** ========== 你已有的功能 ========== */
	void CreateSimplePCGGraph();
	void CreatePCGGraphNodesFromJSON();
	void GenerateBlueprintsFromSpawnActorJSON();
	void SearchFabModelsFromJSON();
	void ImportFbx();
	void ImportGLTF();
	void TakeModelPicture();
	void SaveDemo();
	void GeneratePCGGraphLog();
	void GenerateRuleStruct();

	void CreatePCGGraphNodesFromJSON(
		const FString& JsonFilePath_Nodes,
		const FString& JsonFilePath_Connections
	);

	void GenerateBlueprintsFromSpawnActorJSON(
		const FString& SpawnJsonPath,
		const FString& SelectedModelJsonPath,
		const FString& InteractiveAnalyzerJsonPath
	);

	void LinkPCGGraphNodesFromJSON(const FString& JsonFilePath);

	void GeneratePCGGraphLogFromJSON();
	void GeneratePCGGraphLogFromJSON(
		const FString& InJsonFilePath,
		const FString& OutLogFilePath
	);

	void SearchFabModelsFromJSON(
		const FString& JsonFilePath,
		const FString& SketchfabApiToken
	);

private:

	FHttpRouteHandle RouteHandle;
	TSharedPtr<IHttpRouter> HttpRouter;
	bool bServerStarted = false;
	int32 ServerPort = 8080;

	/** 保存窗口引用 */
	TWeakPtr<SWindow> LLMWindow;
};