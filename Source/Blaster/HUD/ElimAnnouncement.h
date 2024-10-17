// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

UCLASS()
class BLASTER_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	void AddElimMessage(const FString& AttackerName, const FString& VictimName);
	void ClearElimMessage();
private:
	UPROPERTY(meta=(BindWidget))
	class UHorizontalBox* AnnouncementBox;
	
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* AnnouncementText;

	// todo 可以扩展成ShooterGame那种 存一个struct
	TArray<FString> ElimMessage;

	UPROPERTY(EditAnywhere)
	int32 MaxElimMsg = 5;

	FText GetElimAnnouncementText() const;
};
