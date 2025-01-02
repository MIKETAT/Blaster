// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInfo
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time = 0.f;

	UPROPERTY()
	TMap<FName, FBoxInfo> HitBoxInfo;

	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter = nullptr;

	UPROPERTY()
	bool bValidFrame = false;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
public:
	const FString ToString() const
	{
		if (bHitConfirmed == false)
		{
			return FString("Missed!");
		}
		if (bHeadShot)
		{
			return FString("HeadShot!");
		} else
		{
			return FString("Hit!");
		}
	} 
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> HeadShots;

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> BodyShots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);
	void ShowHitBox(const FFramePackage& Package, const FName& BoneName, const FColor& Color);
	void SaveFramePackage(FFramePackage& Package);
	
	FFramePackage InterpFrame(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	FFramePackage GetCheckFrame(ABlasterCharacter* HitCharacter, const float HitTime);
	void SaveFrame();

	// HitScan
	UFUNCTION(Server, Reliable)
	void ServerRequestScore(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, class AWeapon* DamageCauser);

	UFUNCTION(Server, Reliable)
	void ShotgunServerRequestScore(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, const float HitTime, AWeapon* DamageCauser);

	UFUNCTION(Server, Reliable)
	void ProjectileServerRequestScore(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, const float HitTime);
protected:
	virtual void BeginPlay() override;
	void CacheCurrentFrame(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveToPosition(ABlasterCharacter* HitCharacter, const FFramePackage& Position, bool bReset);
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, const ECollisionEnabled::Type Type);
	void ResetPositionAndCollision(ABlasterCharacter* HitCharacter, const FFramePackage& CurrentFrame);

	/**
	 * HitScan
	 */
	FServerSideRewindResult ConfirmRewindResult(const FFramePackage& CheckFrame, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation);
	FServerSideRewindResult ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime);

	/**
	 * Shotgun
	 */
	FShotgunServerSideRewindResult ShotgunConfirmRewindResult(
		const TArray<FFramePackage>& CheckFrames,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations);
	
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		const float HitTime);

	/**
	 * Projectile
	 */
	FServerSideRewindResult ProjectileConfirmRewindResult(const FFramePackage& CheckFrame, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, const float HitTime);
	FServerSideRewindResult ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, const float HitTime);
	
private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
		
};
