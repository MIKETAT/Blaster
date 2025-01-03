// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class USniperScopeOverlayWidget;

USTRUCT()
struct FHUDPackage
{
	GENERATED_BODY()
public:
	FHUDPackage()
	{
		CrosshairCenter = nullptr;
		CrosshairLeft = nullptr;
		CrosshairRight = nullptr;
		CrosshairTop = nullptr;
		CrosshairBottom = nullptr;
		CrosshairSpread = 0.f;
		CrosshairsColor = FColor::Green;
	}
	class UTexture2D* CrosshairCenter;
	UTexture2D* CrosshairLeft ;
	UTexture2D* CrosshairRight;
	UTexture2D* CrosshairTop;
	UTexture2D* CrosshairBottom;
	float CrosshairSpread;
	FLinearColor CrosshairsColor;
};


UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;		// set from blueprint

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<UUserWidget> AnnouncementClass;

	UPROPERTY()
	class UAnnouncement* Announcement;

	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<class UElimAnnouncement> ElimAnnouncementClass;

	UPROPERTY()
	UElimAnnouncement* ElimAnnouncement;

	UPROPERTY(EditAnywhere, Category = "Sniper Scope")
	TSubclassOf<UUserWidget> SniperScopeClass;

	UPROPERTY()
	USniperScopeOverlayWidget* SniperScopeOverlayWidget;

	UPROPERTY(EditAnywhere)
	class USoundCue* ZoomInSound;

	UPROPERTY(EditAnywhere)
	USoundCue* ZoomOutSound;

	void AddCharacterOverlay();
	bool AddAnnouncement();
	void AddElimAnnouncement(const FString& Attacker, const FString& Victim);
	void ShowSniperScopeOverlay(bool bShowScope);
	FORCEINLINE void SetHUDCrosshairEnabled(bool bEnable) { bCrosshairEnabled = bEnable; }
	FORCEINLINE bool GetHUDCrosshairEnabled() const { return bCrosshairEnabled; }
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove);
private:
	UPROPERTY()
	APlayerController* OwningController;
	
	FHUDPackage HUDPackage;

	bool bCrosshairEnabled = false;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 15.f;

	FTimerHandle ElimAnnouncementHandle;

	FTimerDelegate ElimAnnouncementDelegate;

	UPROPERTY(EditAnywhere)
	float ElimAnnouncementExistTime = 5.f;
	
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairsColor);
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package)
	{
		HUDPackage = Package;
		bCrosshairEnabled = true;
	}
};
