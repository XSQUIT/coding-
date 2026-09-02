// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZeldaLike3DGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AZeldaLike3DGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AZeldaLike3DGameMode();
};



