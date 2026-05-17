// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ThirdPersonTemplateGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AThirdPersonTemplateGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AThirdPersonTemplateGameMode();
};



