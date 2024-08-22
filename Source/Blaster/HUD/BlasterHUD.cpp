// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"

#include "CharacterOverlay.h"
#include "Blueprint/UserWidget.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();
	AddCharacterOverlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();
	if (GEngine)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
		const float ScaledSpread = CrosshairSpreadMax * HUDPackage.CrosshairSpread;
		DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, FVector2D(0.f, 0.f), HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, FVector2D(0.f, -ScaledSpread), HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, FVector2D(0.f, ScaledSpread), HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, FVector2D(-ScaledSpread, 0.f), HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, FVector2D(ScaledSpread, 0.f), HUDPackage.CrosshairsColor);
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairsColor)
{
	if (Texture == nullptr)	return;
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);

	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		CrosshairsColor
	);
}
