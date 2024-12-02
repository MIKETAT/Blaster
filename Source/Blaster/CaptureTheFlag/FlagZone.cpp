// Fill out your copyright notice in the Description page of Project Settings.


#include "FlagZone.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameMode/CaptureTheFlagGameMode.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Blaster/Weapon/Flag.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

class ACaptureTheFlagGameMode;

AFlagZone::AFlagZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetRelativeScale3D(FVector(4.f, 4.f, 1.f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SetRootComponent(Mesh);

	Zone = CreateDefaultSubobject<USphereComponent>(TEXT("Zone"));
	Zone->SetRelativeScale3D(FVector(6.f, 6.f, 6.f));
	Zone->SetupAttachment(Mesh);
	Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zone->SetCollisionResponseToAllChannels(ECR_Ignore);
	Zone->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
}

void AFlagZone::BeginPlay()
{
	Super::BeginPlay();
	Zone->OnComponentBeginOverlap.AddDynamic(this, &AFlagZone::OnSphereOverlap);
}

void AFlagZone::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	/*DebugUtil::PrintMsg(FString(TEXT("AFlagZone::OnSphereOverlap")));
	DebugUtil::PrintMsg(Cast<ABlasterCharacter>(OtherActor), FString::Printf(TEXT("Overlap Character is %s"), *OtherActor->GetName()));
	DebugUtil::PrintMsg( FString::Printf(TEXT("Overlap Actor is %s"), *OtherActor->GetName()));*/
	AFlag* Flag = Cast<AFlag>(OtherActor);
	if (!Flag)
	{
		DebugUtil::PrintMsg(FString(TEXT("Flag is null")));
		return;
	}
	if (Flag && Flag->GetOwner() == nullptr)
	{
		DebugUtil::PrintMsg(Flag, FString::Printf(TEXT("Flag->GetOwner() == nullptr")));
	}
	
	//if (Flag->GetTeam() != Team) DebugUtil::PrintMsg(FString(TEXT("Not The Same Team")));
	if (Flag && Flag->GetTeam() == Team)	
	{
		ACaptureTheFlagGameMode* CaptureFlagGameMode = GetWorld()->GetAuthGameMode<ACaptureTheFlagGameMode>();
		if (CaptureFlagGameMode)
		{
			CaptureFlagGameMode->FlagCaptured(Flag, this);
		}
		UGameplayStatics::SpawnSoundAttached(FlagResetSound, Flag->GetRootComponent());
		Flag->ResetFlag();
	}
}
