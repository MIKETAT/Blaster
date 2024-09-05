// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SniperScopeOverlayWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API USniperScopeOverlayWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ScopeZoomIn;
};
