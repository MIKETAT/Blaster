// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"

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
		Pair.Value->SetWorldLocation(Position.HitBoxInfo[Name].Location);
		Pair.Value->SetWorldRotation(Position.HitBoxInfo[Name].Rotation);
		Pair.Value->SetBoxExtent(Position.HitBoxInfo[Name].BoxExtent);
		if (bReset)
		{
			Pair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
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
FServerSideRewindResult ULagCompensationComponent::ConfirmRewindResult(const FFramePackage& CheckFrame, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr)	return FServerSideRewindResult();
	// Save Current Frame
	FFramePackage CurrentFrame;
	CacheCurrentFrame(HitCharacter, CurrentFrame);
	// Move to rewind Position
	MoveToPosition(HitCharacter, CheckFrame, false);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::Type::NoCollision);
	// Check
	UWorld* World = GetWorld();
	if (!World)		return FServerSideRewindResult();
	// is headshot?
	UBoxComponent* Head = HitCharacter->HitCollisionBoxes[FName("head")];
	Head->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Head->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	FHitResult ConfirmHit;
	World->LineTraceSingleByChannel(ConfirmHit, TraceStart, TraceEnd, ECC_Visibility);
	if (ConfirmHit.bBlockingHit)	// headshot
	{
		ResetPositionAndCollision(HitCharacter, CurrentFrame);
		return FServerSideRewindResult{true, true};
	}
	// no headshot
	for (const auto& Pair : HitCharacter->HitCollisionBoxes)
	{
		if (Pair.Value)
		{
			Pair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Pair.Value->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
	}
	World->LineTraceSingleByChannel(ConfirmHit, TraceStart, TraceEnd, ECC_Visibility);
	if (ConfirmHit.bBlockingHit)
	{
		ResetPositionAndCollision(HitCharacter, CurrentFrame);
		return FServerSideRewindResult{true, false};
	}
	ResetPositionAndCollision(HitCharacter, CurrentFrame);
	return FServerSideRewindResult{false, false};
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

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (!Character || !GetWorld())		return;
	Package.Time = GetWorld()->GetTimeSeconds();
	for (const auto& Pair : Character->HitCollisionBoxes)
	{
		FBoxInfo BoxInfo;
		BoxInfo.Location = Pair.Value->GetComponentLocation();
		BoxInfo.Rotation = Pair.Value->GetComponentRotation();
		BoxInfo.BoxExtent = Pair.Value->GetScaledBoxExtent();
		Package.HitBoxInfo.Add(Pair.Key, BoxInfo);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, const float HitTime)
{
	bool bReturn = HitCharacter == nullptr || HitCharacter->GetLagCompensationComponent() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr;
	if (bReturn)
	{
		DebugUtil::PrintMsg(TEXT("ServerSideRewind return early bReturn == true"));
		return FServerSideRewindResult();
	}
	FFramePackage CheckFrame;
	bool bShouldInterpolate = true;
	const auto& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float YoungestHistoryTime = History.GetHead()->GetValue().Time;
	// out of History
	if (HitTime < OldestHistoryTime)
	{
		DebugUtil::PrintMsg(TEXT("ServerSideRewind return early HitTime < OldestHistoryTime"));
		return FServerSideRewindResult();		// too old, no records			
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
	return ConfirmRewindResult(CheckFrame, HitCharacter, TraceStart, HitLocation);
}

FFramePackage ULagCompensationComponent::InterpFrame(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	FFramePackage InterpFrame;
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

