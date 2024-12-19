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
	AFlag* Flag = Cast<AFlag>(OtherActor);
	if (Flag && Flag->GetTeam() == Team)	
	{
		if (ACaptureTheFlagGameMode* CaptureFlagGameMode = GetWorld()->GetAuthGameMode<ACaptureTheFlagGameMode>())
		{
			CaptureFlagGameMode->FlagCaptured(Flag, this);
		}
		UGameplayStatics::SpawnSoundAttached(FlagResetSound, Flag->GetRootComponent());
		Flag->ResetFlag();
	}
}
