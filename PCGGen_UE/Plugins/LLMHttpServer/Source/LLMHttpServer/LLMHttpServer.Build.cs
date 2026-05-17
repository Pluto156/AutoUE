using System.IO;
using UnrealBuildTool;

public class LLMHttpServer : ModuleRules
{
    public LLMHttpServer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "LLMEditor/Private/LLMGenerate",
                "LLMEditor/Public/Core/Interact",
                "LLMEditor/Public/Core/ModuleManager",
                "LLMEditor/Public/Core/Combat",
                "LLMEditor/Public/ThirdPersonTemplate",
                "LLMEditor/Public/CustomPCGNode",
            }
        );

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
                "UMG"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "AssetTools",
                "AssetRegistry",
                "EditorSubsystem",
                "ToolMenus",
                "ApplicationCore",
                "Json",
                "JsonUtilities",
                "Projects",

                "PCG",
                "PCGEditor",

                "HTTP",
                "SSL",

                "LLMEditor",

                "FBX",


        "InterchangeCore",
"InterchangeEngine",
"InterchangeImport",
"InterchangePipelines",
"GLTFCore",
"InterchangeCommonParser",
"RenderCore",
"RHI"
            }
        );
    }
}
