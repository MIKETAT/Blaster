// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInfo> HitBoxInfo;
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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);
	void SaveFramePackage(FFramePackage& Package);
	FServerSideRewindResult ServerSideRewind(class ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime);
	FFramePackage InterpFrame(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	void SaveFrame();
	UFUNCTION(Server, Reliable)
	void ServerRequestScore(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, class AWeapon* DamageCauser);
protected:
	virtual void BeginPlay() override;
	void CacheCurrentFrame(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveToPosition(ABlasterCharacter* HitCharacter, const FFramePackage& Position, bool bReset);
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, const ECollisionEnabled::Type Type);
	void ResetPositionAndCollision(ABlasterCharacter* HitCharacter, const FFramePackage& CurrentFrame);
	FServerSideRewindResult ConfirmRewindResult(const FFramePackage& CheckFrame, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation);
private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
		
};
