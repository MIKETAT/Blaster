#include "PickupSpawnPoint.h"

#include "Pickup.h"
#include "Kismet/GameplayStatics.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	StartSpawnPickupTimer(nullptr);
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::RandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	GetWorldTimerManager().SetTimer(
		PickupSpawnTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinish,
		SpawnTime);
}

// 中间层，相比直接bind RandomSpawnPickup 更灵活
void APickupSpawnPoint::SpawnPickupTimerFinish()
{
	if (HasAuthority())
	{
		RandomSpawnPickup();	
	}
}

void APickupSpawnPoint::Tick(float DeltaTime)
{		
	Super::Tick(DeltaTime);
}

void APickupSpawnPoint::RandomSpawnPickup()
{
	if (!HasAuthority())	return;
	// get random pickup class
	int32 NumOfPickupClasses = PickupClasses.Num();
	if (NumOfPickupClasses <= 0)	return;
	int Selection = FMath::RandRange(0, NumOfPickupClasses-1);
	SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());
	if (HasAuthority() && SpawnedPickup)
	{
		SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
	}
}

