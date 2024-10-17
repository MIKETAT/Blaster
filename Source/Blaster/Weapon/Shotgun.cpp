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

		TMap<ABlasterCharacter*, uint32> HitMap;
		TMap<ABlasterCharacter*, uint32> HeadShotHitMap;
		TMap<ABlasterCharacter*, float> DamageMap;
		
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult HitResult;
			WeaponHit(Start, HitTarget, HitResult);
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
			if (BlasterCharacter == nullptr)	continue;
			if (HitResult.BoneName == FString("head"))
			{
				if (HeadShotHitMap.Contains(BlasterCharacter))	HeadShotHitMap[BlasterCharacter]++;
				else HeadShotHitMap.Emplace(BlasterCharacter, 1);
			} else
			{
				if (HitMap.Contains(BlasterCharacter))	HitMap[BlasterCharacter]++;
				else HitMap.Emplace(BlasterCharacter, 1);
			}
		}

		for (auto Pair : HitMap)
		{
			DamageMap[Pair.Key] += Damage * Pair.Value;
		}
		for (auto Pair : HeadShotHitMap)
		{
			DamageMap[Pair.Key] += HeadShotDamage * Pair.Value;
		}
		if (InstigatorController && HasAuthority() && BlasterController)
		{
			for (auto Pair : DamageMap)
			{
				UGameplayStatics::ApplyDamage(Pair.Key, Pair.Value, InstigatorController, this, UDamageType::StaticClass());
			}
		}
	}
}