// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"

#include "Announcement.h"
#include "CharacterOverlay.h"
#include "Blaster/HUD/ElimAnnouncement.h"
#include "Blueprint/UserWidget.h"
#include "SniperScopeOverlayWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController == nullptr)	return;
	if (CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
	}
	if (AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
	}
	if (SniperScopeClass)
	{
		SniperScopeOverlayWidget = CreateWidget<USniperScopeOverlayWidget>(PlayerController, SniperScopeClass);
	}
}

void ABlasterHUD::ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove)
{
	if (MsgToRemove)
	{
		MsgToRemove->RemoveFromParent();
		MsgToRemove->ClearElimMessage();
	}
}

// todo 变量初始化时机，有更合适的实现吗
void ABlasterHUD::AddCharacterOverlay()
{
	if (CharacterOverlay)
	{
		CharacterOverlay->AddToViewport();	
	}
	/*if (CharacterOverlay == nullptr && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
	}*/
}

bool ABlasterHUD::AddAnnouncement()
{
	if (Announcement)
	{
		Announcement->AddToViewport();
		return true;
	}
	return false;
}

void ABlasterHUD::AddElimAnnouncement(const FString& Attacker, const FString& Victim)
{
	if (!ElimAnnouncementClass)	return;
	OwningController = OwningController == nullptr ? GetOwningPlayerController() : OwningController;
	if (!ElimAnnouncement)
	{
		ElimAnnouncement = CreateWidget<UElimAnnouncement>(OwningController, ElimAnnouncementClass);	
	}
	if (ElimAnnouncement)
	{
		ElimAnnouncement->AddElimMessage(Attacker, Victim);
		ElimAnnouncementDelegate.BindUFunction(this, FName("ElimAnnouncementTimerFinished"), ElimAnnouncement);
		if (GetWorldTimerManager().IsTimerActive(ElimAnnouncementHandle))
		{
			GetWorldTimerManager().ClearTimer(ElimAnnouncementHandle);	// reset
		}
		GetWorldTimerManager().SetTimer(
			ElimAnnouncementHandle,
			ElimAnnouncementDelegate,
			ElimAnnouncementExistTime,
			false);
		if (!ElimAnnouncement->IsInViewport())
		{
			ElimAnnouncement->AddToViewport();	
		}
	}
}

void ABlasterHUD::ShowSniperScopeOverlay(bool bShowScope)
{
	if (!SniperScopeOverlayWidget || !SniperScopeOverlayWidget->ScopeZoomIn)	return;
	if (bShowScope)
	{
		SniperScopeOverlayWidget->SetVisibility(ESlateVisibility::Visible);
		SniperScopeOverlayWidget->PlayAnimationForward(SniperScopeOverlayWidget->ScopeZoomIn);
		UGameplayStatics::PlaySound2D(GetWorld(), ZoomInSound);
	} else
	{
		// 先播放动画，再Hide
		SniperScopeOverlayWidget->PlayAnimationReverse(SniperScopeOverlayWidget->ScopeZoomIn);
		UGameplayStatics::PlaySound2D(GetWorld(), ZoomOutSound);
		SniperScopeOverlayWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();
	if (GEngine && GetHUDCrosshairEnabled())
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
