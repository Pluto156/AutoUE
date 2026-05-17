// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "LLMEditorStyle.h"

class FLLMEditorCommands : public TCommands<FLLMEditorCommands>
{
public:

	FLLMEditorCommands()
		: TCommands<FLLMEditorCommands>(TEXT("LLMEditor"), NSLOCTEXT("Contexts", "LLMEditor", "LLMEditor Plugin"), NAME_None, FLLMEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
