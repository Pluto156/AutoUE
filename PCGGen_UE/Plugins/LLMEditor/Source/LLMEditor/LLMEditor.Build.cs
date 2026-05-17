// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LLMEditor : ModuleRules
{
	public LLMEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // =====================================
        // Include 路径
        // =====================================
        PrivateIncludePaths.AddRange(
            new string[]
            {
                "LLMEditor/Private/LLMGenerate",
                "LLMEditor/Public/Core/Interact",
                "LLMEditor/Public/Core/ModuleManager",
                "LLMEditor/Public/Core/Combat",
                "LLMEditor/Public/Core/EvaluateExecutor",
                "LLMEditor/Public/ThirdPersonTemplate",
                "LLMEditor/Public/CustomPCGNode",
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {

            }
        );

        // =====================================
        // Public 依赖模块（仅暴露接口，不链接 .lib）
        // =====================================
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "HTTPServer",
                "Slate",
                "SlateCore",
                "EnhancedInput",
                "UMG",
                "UnrealEd",     // ✅ 必须有这个才能注册菜单
            "ToolMenus" ,    // ✅ 注册菜单系统
            "AIModule",
             "NavigationSystem",
                     "InputCore",
            }
        );

        // =====================================
        // Private 依赖模块（这些才会实际链接）
        // =====================================
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",        // ✅ 提供 UEdGraph、资产保存等
                "EditorSubsystem", // ✅ 编辑器管理
                "GraphEditor",     // ✅ 图形编辑 API
                "ToolMenus",
                "ApplicationCore",
                "Json",
                "JsonUtilities",
                "Projects",
                "PCG",             // ✅ PCG Runtime
                "PCGEditor" ,
                            "AIModule",
             "NavigationSystem",
             "EditorScriptingUtilities", // ✅ 这里添加 EditorAssetLibrary 所在模块
        "AssetRegistry",            // ✅ 通常也需要注册资产
            }
        );
    }
}
