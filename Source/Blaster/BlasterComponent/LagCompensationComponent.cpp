// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"

#include "Blaster/Blaster.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SaveFrame();
}


// Cache Current CollisionBoxes
void ULagCompensationComponent::CacheCurrentFrame(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr)	return;
	for (const auto& Pair : HitCharacter->HitCollisionBoxes)
	{
		if (Pair.Value != nullptr)
		{
			FBoxInfo BoxInfo;
			BoxInfo.Location = Pair.Value->GetComponentLocation();
			BoxInfo.Rotation = Pair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = Pair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(Pair.Key, BoxInfo);
		}
	}
}

// Move Character CollisionBoxes to Position
void ULagCompensationComponent::MoveToPosition(ABlasterCharacter* HitCharacter, const FFramePackage& Position, bool bReset)
{
	if (HitCharacter == nullptr)	return;
	for (auto& Pair : HitCharacter->HitCollisionBoxes)
	{
		FName Name = Pair.Key;
		if (Pair.Value)
		{
			DebugUtil::PrintMsg(TEXT("Pair.Value is null"));
		}
		Pair.Value->SetWorldLocation(Position.HitBoxInfo[Name].Location);
		Pair.Value->SetWorldRotation(Position.HitBoxInfo[Name].Rotation);
		Pair.Value->SetBoxExtent(Position.HitBoxInfo[Name].BoxExtent);
		if (bReset)
		{
			Pair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		UBoxComponent* HitBox = Pair.Value;
		DrawDebugBox(GetWorld(),
					HitBox->GetComponentLocation(),
					HitBox->GetScaledBoxExtent(),
					FQuat(HitBox->GetComponentRotation()),
					FColor::Orange,
					false,
					8.f);
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, const ECollisionEnabled::Type Type)
{
	if (HitCharacter == nullptr || HitCharacter->GetMesh() == nullptr)	return;
	HitCharacter->GetMesh()->SetCollisionEnabled(Type);
}

void ULagCompensationComponent::ResetPositionAndCollision(ABlasterCharacter* HitCharacter, const FFramePackage& CurrentFrame)
{
	if (HitCharacter == nullptr)	return;
	MoveToPosition(HitCharacter, CurrentFrame, true);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::Type::QueryAndPhysics);
}

// Server Rewind to Confirm whether Hit
FServerSideRewindResult ULagCompensationComponent::ConfirmRewindResult(const FFramePackage& CheckFrame, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (CheckFrame.BlasterCharacter == nullptr)	return FServerSideRewindResult();
	// Save Current Frame
	FFramePackage CurrentFrame;
	CacheCurrentFrame(CheckFrame.BlasterCharacter, CurrentFrame);
	// Move to rewind Position
	MoveToPosition(CheckFrame.BlasterCharacter, CheckFrame, false);
	EnableCharacterMeshCollision(CheckFrame.BlasterCharacter, ECollisionEnabled::Type::NoCollision);
	// Check
	UWorld* World = GetWorld();
	if (!World)		return FServerSideRewindResult();
	// is headshot?
	UBoxComponent* Head = CheckFrame.BlasterCharacter->HitCollisionBoxes[FName("head")];
	Head->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Head->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);

	FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	FHitResult ConfirmHit;
	World->LineTraceSingleByChannel(ConfirmHit, TraceStart, TraceEnd, ECC_HitBox);
	if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(ConfirmHit.GetActor()))	// headshot
	{
		ResetPositionAndCollision(CheckFrame.BlasterCharacter, CurrentFrame);
		return FServerSideRewindResult{true, true};
	}
	// no headshot
	for (const auto& Pair : CheckFrame.BlasterCharacter->HitCollisionBoxes)
	{
		if (Pair.Value)
		{
			Pair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Pair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
		}
	}
	World->LineTraceSingleByChannel(ConfirmHit, TraceStart, TraceEnd, ECC_HitBox);
	if (ConfirmHit.bBlockingHit)
	{
		// show hit box
		if (ConfirmHit.Component.IsValid())
		{
			if (UBoxComponent* HitBox = Cast<UBoxComponent>(ConfirmHit.Component))
			{
				DrawDebugBox(GetWorld(),
					HitBox->GetComponentLocation(),
					HitBox->GetScaledBoxExtent(),
					FQuat(HitBox->GetComponentRotation()),
					FColor::Red,
					false,
					8.f);
			}
		}
		
		ResetPositionAndCollision(CheckFrame.BlasterCharacter, CurrentFrame);
		return FServerSideRewindResult{true, false};
	}
	ResetPositionAndCollision(CheckFrame.BlasterCharacter, CurrentFrame);
	return FServerSideRewindResult{false, false};
}

void ULagCompensationComponent::SaveFrame()
{
	FFramePackage ThisFrame;
	SaveFramePackage(ThisFrame);
	if (FrameHistory.Num() <= 1)
	{
		FrameHistory.AddHead(ThisFrame);
	} else
	{
		float RecordTime = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (RecordTime > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			RecordTime = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FrameHistory.AddHead(ThisFrame);
	}
	//ShowFramePackage(ThisFrame, FColor::Red);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (!Character || !GetWorld())		return;
	Package.Time = GetWorld()->GetTimeSeconds();
	Package.BlasterCharacter = Character;
	for (const auto& Pair : Character->HitCollisionBoxes)
	{
		FBoxInfo BoxInfo;
		BoxInfo.Location = Pair.Value->GetComponentLocation();
		BoxInfo.Rotation = Pair.Value->GetComponentRotation();
		BoxInfo.BoxExtent = Pair.Value->GetScaledBoxExtent();
		Package.HitBoxInfo.Add(Pair.Key, BoxInfo);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (const auto& Pair : Package.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(),
			Pair.Value.Location,
			Pair.Value.BoxExtent,
			FQuat(Pair.Value.Rotation),
			Color,
			false,
			MaxRecordTime);
	}
}

FFramePackage ULagCompensationComponent::GetCheckFrame(ABlasterCharacter* HitCharacter, const float HitTime)
{
	bool bReturn = HitCharacter == nullptr || HitCharacter->GetLagCompensationComponent() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr;
	if (bReturn)
	{
		DebugUtil::PrintMsg(TEXT("GetCheckFrame return early"));
		return FFramePackage();
	}
	FFramePackage CheckFrame;
	CheckFrame.BlasterCharacter = HitCharacter;
	bool bShouldInterpolate = true;
	const auto& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float YoungestHistoryTime = History.GetHead()->GetValue().Time;
	// out of History
	if (HitTime < OldestHistoryTime)
	{
		DebugUtil::PrintMsg(TEXT("ServerSideRewind return early HitTime < OldestHistoryTime"));
		return FFramePackage();		// too old, no records			
	}
	if (HitTime == OldestHistoryTime)
	{
		CheckFrame = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	} else if (HitTime >= YoungestHistoryTime)
	{
		// not possible, may be on server
		CheckFrame = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	} else // in History	
	{
		auto Younger = History.GetHead();
		auto Older = Younger;
		while (Older->GetValue().Time > HitTime)
		{
			if (Older->GetNextNode() == nullptr)	break;
			Older = Older->GetNextNode();
			if (Older->GetValue().Time > HitTime)
			{
				Younger = Older;
			}
		}
		if (Older->GetValue().Time == HitTime)
		{
			CheckFrame = Older->GetValue();
			bShouldInterpolate = false;
		}
		if (bShouldInterpolate)
		{
			CheckFrame = InterpFrame(Older->GetValue(), Younger->GetValue(), HitTime);
		}
	}
	CheckFrame.bValidFrame = true;
	return CheckFrame;
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime)
{
	FFramePackage CheckFrame = GetCheckFrame(HitCharacter, HitTime);
	return ConfirmRewindResult(CheckFrame, TraceStart, HitLocation);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, const float HitTime)
{
	TArray<FFramePackage> CheckFrames;
	for (auto HitCharacter : HitCharacters)
	{
		CheckFrames.Add(GetCheckFrame(HitCharacter, HitTime));
	}
	return ShotgunConfirmRewindResult(CheckFrames, TraceStart, HitLocations);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmRewindResult(const FFramePackage& CheckFrame, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, const float HitTime)
{
	if (CheckFrame.BlasterCharacter == nullptr)	return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheCurrentFrame(CheckFrame.BlasterCharacter, CurrentFrame);
	MoveToPosition(CheckFrame.BlasterCharacter, CheckFrame, false);
	EnableCharacterMeshCollision(CheckFrame.BlasterCharacter, ECollisionEnabled::Type::NoCollision);

	// check headshot
	UBoxComponent* HeadBox = CheckFrame.BlasterCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
	
	FPredictProjectilePathParams Params;
	Params.bTraceWithCollision = true;
	Params.MaxSimTime = MaxRecordTime;
	Params.LaunchVelocity = InitialVelocity;
	Params.StartLocation = TraceStart;
	Params.SimFrequency = 15.f;
	Params.TraceChannel = ECC_HitBox;
	Params.ActorsToIgnore.Add(GetOwner());
	Params.DrawDebugTime = 5.f;
	Params.DrawDebugType = EDrawDebugTrace::ForDuration;
	
	FPredictProjectilePathResult Result;
	UGameplayStatics::PredictProjectilePath(this, Params, Result);

	if (Result.HitResult.bBlockingHit)
	{
		if (Result.HitResult.Component.IsValid())
		{
			if (UBoxComponent* Box = Cast<UBoxComponent>(Result.HitResult.GetActor()))
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FColor::Red, false, 5.f);
			}
		}

		MoveToPosition(CheckFrame.BlasterCharacter, CurrentFrame, true);
		EnableCharacterMeshCollision(CheckFrame.BlasterCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}
		
	// no headshot, check body shot
	for (auto& Pair : CheckFrame.BlasterCharacter->HitCollisionBoxes)
	{
		if (Pair.Value)
		{
			Pair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Pair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
		}
	}
	
	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, Result);
	if (Result.HitResult.bBlockingHit)
	{
		if (Result.HitResult.Component.IsValid())
		{
			if (UBoxComponent* Box = Cast<UBoxComponent>(Result.HitResult.GetActor()))
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FColor::Blue, false, 5.f);
			}
		}
		
		MoveToPosition(CheckFrame.BlasterCharacter, CurrentFrame, true);
		EnableCharacterMeshCollision(CheckFrame.BlasterCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, false };
	}

	MoveToPosition(CheckFrame.BlasterCharacter, CurrentFrame, true);
	EnableCharacterMeshCollision(CheckFrame.BlasterCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter,
                                                                              const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage CheckFrame = GetCheckFrame(HitCharacter, HitTime);
	return ProjectileConfirmRewindResult(CheckFrame, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmRewindResult(
	const TArray<FFramePackage>& CheckFrames, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations)
{
	FShotgunServerSideRewindResult Result;
	// check is valid
	for (const auto& Frame : CheckFrames)
	{
		if (Frame.BlasterCharacter == nullptr)	return Result;
	}

	// Cache Current Frames, one by one
	TArray<FFramePackage> CurrentFrames;
	for (const auto& Frame : CheckFrames)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.BlasterCharacter = Frame.BlasterCharacter;
		CacheCurrentFrame(Frame.BlasterCharacter, CurrentFrame);
		MoveToPosition(CurrentFrame.BlasterCharacter, CurrentFrame, false);
		EnableCharacterMeshCollision(Frame.BlasterCharacter, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);
		// enabel head box for HeadShot check
		UBoxComponent* Head = Frame.BlasterCharacter->HitCollisionBoxes[FName("head")];
		Head->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Head->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
	}

	UWorld* World = GetWorld();
	if (!World)		return Result;
	
	// linetrace for HeadShots
	for (const auto& HitLocation : HitLocations)
	{
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		FHitResult HeadShotHitResult;
		World->LineTraceSingleByChannel(HeadShotHitResult, TraceStart, TraceEnd, ECC_HitBox);
		if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(HeadShotHitResult.GetActor()))
		{
			if (Result.HeadShots.Contains(HitCharacter))	Result.HeadShots[HitCharacter]++;
			else	Result.HeadShots.Emplace(HitCharacter, 1);
		}
	}

	// line trace for body shots, 
	// enable all box components, and disable head box
	for (const auto& Frame : CheckFrames)
	{
		for (const auto& Pair : Frame.BlasterCharacter->HitCollisionBoxes)
		{
			if (!Pair.Value)	continue;
			Pair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Pair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
		}
		// disable head box
		UBoxComponent* Head = Frame.BlasterCharacter->HitCollisionBoxes[FName("head")];
		Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// check body shots
	for (const auto& HitLocation : HitLocations)
	{
		FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		FHitResult HeadShotHitResult;
		World->LineTraceSingleByChannel(HeadShotHitResult, TraceStart, TraceEnd, ECC_HitBox);
		if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(HeadShotHitResult.GetActor()))
		{
			if (Result.BodyShots.Contains(HitCharacter))	Result.BodyShots[HitCharacter]++;
			else	Result.BodyShots.Emplace(HitCharacter, 1);
		}
	}
	// reset
	for (const auto& CurrentFrame : CurrentFrames)
	{
		MoveToPosition(CurrentFrame.BlasterCharacter, CurrentFrame, true);
		EnableCharacterMeshCollision(CurrentFrame.BlasterCharacter, ECollisionEnabled::QueryAndPhysics);
	}
	return Result;
}

FFramePackage ULagCompensationComponent::InterpFrame(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	FFramePackage InterpFrame;
	InterpFrame.BlasterCharacter = OlderFrame.BlasterCharacter;
	InterpFrame.Time = HitTime;
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float Fraction = (HitTime - OlderFrame.Time)  / Distance;
	/*DebugUtil::PrintMsg(FString::Printf(TEXT("InterpFrame, Old HitBoxInfo.Num = %d"), OlderFrame.HitBoxInfo.Num()), FColor::Red);
	DebugUtil::PrintMsg(FString::Printf(TEXT("InterpFrame, Old HitBoxInfo Key = %s"), *OlderFrame.HitBoxInfo.begin().Key().ToString()), FColor::Red);
	*/
	
	for (const auto& Pair : OlderFrame.HitBoxInfo)
	{
		const FName& Name = Pair.Key;
		const FBoxInfo& OlderBoxInfo = OlderFrame.HitBoxInfo[Name];
		const FBoxInfo& YoungerBoxInfo = YoungerFrame.HitBoxInfo[Name];
		FBoxInfo BoxInfo;
		BoxInfo.Location = FMath::VInterpTo(OlderBoxInfo.Location, YoungerBoxInfo.Location, 1, Fraction);
		BoxInfo.Rotation = FMath::RInterpTo(OlderBoxInfo.Rotation, YoungerBoxInfo.Rotation, 1, Fraction);
		BoxInfo.BoxExtent = YoungerBoxInfo.BoxExtent;
		InterpFrame.HitBoxInfo.Add(Name, BoxInfo);
	}
	return InterpFrame;
}


// send a rpc to server to request a score
void ULagCompensationComponent::ServerRequestScore_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime, AWeapon* DamageCauser)
{
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	}
	DebugUtil::PrintMsg(FString(TEXT("Call ServerRequestScore")), FColor::Red);

	FServerSideRewindResult ConfirmResult = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	DebugUtil::PrintMsg(FString::Printf(TEXT("ConfirmResult = %s"), *ConfirmResult.ToString()));
	if (Character == nullptr || Controller == nullptr || DamageCauser == nullptr || HitCharacter == nullptr || ConfirmResult.bHitConfirmed == false)
	{
		DebugUtil::PrintMsg(TEXT("ServerRequest Score, early quit, something is null"));
		if (Character == nullptr)	DebugUtil::PrintMsg(TEXT("Character is null"));
		if (Controller == nullptr)	DebugUtil::PrintMsg(TEXT("Controller is null"));
		if (DamageCauser == nullptr)	DebugUtil::PrintMsg(TEXT("DamageCauser is null"));
		if (HitCharacter == nullptr)	DebugUtil::PrintMsg(TEXT("HitCharacter is null"));
		if (ConfirmResult.bHitConfirmed == false)	DebugUtil::PrintMsg(TEXT("ConfirmResult.bHitConfirmed == false"));
		return;
	}
	float DamageToCause = ConfirmResult.bHeadShot ? DamageCauser->GetHeadshotDamage() : DamageCauser->GetDamage();
	UGameplayStatics::ApplyDamage(
		HitCharacter,
		DamageToCause,
		Controller,
		DamageCauser,
		UDamageType::StaticClass()
	);
	DebugUtil::PrintMsg(FString::Printf(TEXT("Cause Damage %f"), DamageToCause), FColor::Red);
}

void ULagCompensationComponent::ShotgunServerRequestScore_Implementation(const TArray<ABlasterCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, const float HitTime,
	AWeapon* DamageCauser)
{
	// calculate rewind result
	FShotgunServerSideRewindResult RewindResult = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	// apply damage to every character
	for (const auto HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Controller == nullptr || DamageCauser == nullptr) continue;
		float DamageToCause = 0.f;
		if (RewindResult.HeadShots.Contains(HitCharacter))
		{
			DamageToCause += RewindResult.HeadShots[HitCharacter] * Character->GetEquippedWeapon()->GetHeadshotDamage();
		}
		if (RewindResult.BodyShots.Contains(HitCharacter))
		{
			DamageToCause += RewindResult.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
		}
		UGameplayStatics::ApplyDamage(HitCharacter, DamageToCause, Controller, DamageCauser, UDamageType::StaticClass());
	}
	
}

void ULagCompensationComponent::ProjectileServerRequestScore_Implementation(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, const float HitTime)
{
	DebugUtil::PrintMsg(TEXT("ProjectileServerRequestScore"));
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	DebugUtil::PrintMsg(FString::Printf(TEXT("%s"), *Confirm.ToString()));
	if (Character && Character->GetEquippedWeapon() && Confirm.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Character->GetEquippedWeapon()->GetDamage(),
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass());
	}
	if (Character == nullptr)	DebugUtil::PrintMsg(TEXT("Character == nullptr"));
	if (Character->GetEquippedWeapon() == nullptr)	DebugUtil::PrintMsg(TEXT("Character->GetEquippedWeapon()"));
	if (Confirm.bHitConfirmed == false)	DebugUtil::PrintMsg(TEXT("Confirm.bHitConfirmed == false"));

}