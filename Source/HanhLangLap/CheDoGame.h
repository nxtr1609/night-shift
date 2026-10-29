#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CheDoGame.generated.h"

UCLASS()
class ACheDoGame : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACheDoGame();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
};
