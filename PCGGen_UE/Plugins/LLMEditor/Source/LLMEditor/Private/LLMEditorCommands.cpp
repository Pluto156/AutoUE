// Copyright Epic Games, Inc. All Rights Reserved.

#include "LLMEditorCommands.h"

#define LOCTEXT_NAMESPACE "FLLMEditorModule"

void FLLMEditorCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "LLMEditor", "Execute LLMEditor action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
