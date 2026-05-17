// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "HttpRouteHandle.h"
#include "Templates/SharedPointer.h"
class IHttpRouter;
class FLLMEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
private:

	/** 注册编辑器菜单 */
	void RegisterMenus();

	/** 启动 HTTP Server */
	void StartHttpServer();

	/** 停止 HTTP Server */
	void StopHttpServer();

	void CreateSimplePCGGraph();
	void CreatePCGGraphFromJSON();

	void CreatePCGGraphFromJSON(const FString& JsonFilePath);


private:
	TSharedPtr<class FUICommandList> PluginCommands;

	/** 当前路由句柄 */
	FHttpRouteHandle RouteHandle;

	/** 保存路由器实例以便解绑 */
	TSharedPtr<IHttpRouter> HttpRouter;

	/** 标志：是否已启动服务 */
	bool bServerStarted = false;

	/** 监听端口（可按需修改或做成 UI 可配置） */
	int32 ServerPort = 8080;
};
