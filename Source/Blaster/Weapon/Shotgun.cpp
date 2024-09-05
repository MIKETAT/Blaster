// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);	// Animation Spend Ammo
	APawn* InstigatorOwner = Cast<APawn>(GetOwner());
	if (InstigatorOwner == nullptr)		return;
	APlayerController* InstigatorController = Cast<APlayerController>(InstigatorOwner->GetController());
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		UWorld* World = GetWorld();
		if (World == nullptr)	return;

		TMap<ABlasterCharacter*, uint32> HitActor;
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FVector EndLoc = TraceEndWithScatter(Start, HitTarget);
			DrawDebugLine(
				GetWorld(),
				Start,
				EndLoc,
				FColor::Red,
				true);
			// GetHitResult
			FHitResult HitResult;
			WeaponHit(Start, HitTarget, HitResult);
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
			if (BlasterCharacter == nullptr)	continue;
			if (HitActor.Contains(BlasterCharacter))
			{
				HitActor[BlasterCharacter]++;
			} else
			{
				HitActor.Emplace(BlasterCharacter, 1);
			}
		}

		for (auto HitPair : HitActor)
		{
			if (InstigatorController && HasAuthority() && BlasterController)
			{
				UGameplayStatics::ApplyDamage(HitPair.Key, Damage * HitPair.Value, InstigatorController, this, UDamageType::StaticClass());
			}
		}
	}
}