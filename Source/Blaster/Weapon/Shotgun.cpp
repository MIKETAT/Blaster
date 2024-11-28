// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"

#include "WeaponTypes.h"
#include "Blaster/BlasterComponent/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

/*
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
			if (DamageMap.Contains(Pair.Key))	DamageMap[Pair.Key] += Damage * Pair.Value;
			else DamageMap.Add(Pair.Key, Damage * Pair.Value);
		}
		for (auto Pair : HeadShotHitMap)
		{
			if (DamageMap.Contains(Pair.Key))	DamageMap[Pair.Key] += HeadShotDamage * Pair.Value;
			else DamageMap.Add(Pair.Key, HeadShotDamage * Pair.Value);
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
*/

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());	// deal with animation casing and spendRound
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
	
		// calc hit results
		for (auto Target : HitTargets)
		{
			FHitResult ShotgunHitResult;
			WeaponHit(Start, Target, ShotgunHitResult);
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ShotgunHitResult.GetActor());
			if (BlasterCharacter == nullptr)	continue;
			if (ShotgunHitResult.BoneName == FString("head"))
			{
				if (HeadShotHitMap.Contains(BlasterCharacter))	HeadShotHitMap[BlasterCharacter]++;
				else HeadShotHitMap.Emplace(BlasterCharacter, 1);
			} else
			{
				if (HitMap.Contains(BlasterCharacter))	HitMap[BlasterCharacter]++;
				else HitMap.Emplace(BlasterCharacter, 1);
			}
			// sound and particle
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, ShotgunHitResult.ImpactPoint, ShotgunHitResult.Normal.Rotation());
			}
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, ShotgunHitResult.ImpactPoint, ShotgunHitResult.Normal.Rotation());
			}
			DrawDebugSphere(GetWorld(), ShotgunHitResult.Location, 10.f, 10, FColor::Blue);
		}

		TArray<ABlasterCharacter*> HitCharacters;
		
		// calculate damage
		for (auto Pair : HitMap)
		{	
			if (Pair.Key == nullptr)	continue;
			if (DamageMap.Contains(Pair.Key))	DamageMap[Pair.Key] += Damage * Pair.Value;
			else DamageMap.Add(Pair.Key, Damage * Pair.Value);
			HitCharacters.AddUnique(Pair.Key);
		}
		for (auto Pair : HeadShotHitMap)
		{
			if (Pair.Key == nullptr)	continue;
			if (DamageMap.Contains(Pair.Key))	DamageMap[Pair.Key] += HeadShotDamage * Pair.Value;
			else DamageMap.Add(Pair.Key, HeadShotDamage * Pair.Value);
			HitCharacters.AddUnique(Pair.Key);
		}

		// apply damage
		for (auto Pair : DamageMap)
		{
			if (InstigatorController && Pair.Key)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || BlasterOwner->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)	// server 控制的 pawn, 直接造成伤害
				{
					UGameplayStatics::ApplyDamage(Pair.Key, Pair.Value, InstigatorController, this, UDamageType::StaticClass());	
				}
			}
		}

		// 如果在client 上控制的pawn且使用SSR, 发起request
		if (!HasAuthority() && bUseServerSideRewind)
		{
			BlasterOwner = BlasterOwner == nullptr ? Cast<ABlasterCharacter>(InstigatorOwner) : BlasterOwner;
			BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterController;
			if (BlasterOwner && BlasterController && BlasterOwner->GetLagCompensationComponent() && BlasterOwner->IsLocallyControlled())
			{
				BlasterOwner->GetLagCompensationComponent()->ShotgunServerRequestScore(HitCharacters,
					Start,
					HitTargets,
					BlasterController->GetServerTime() - BlasterController->GetSingleTripTime(),
					this);
			}
		}
		
	}
}

void AShotgun::ShotgunTraceWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	// 获取枪口位置":
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (!MuzzleFlashSocket)		return;
	const FVector TraceStart = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	
	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandomVec = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, SphereRadius);
		const FVector RandomEnd = SphereCenter + RandomVec;
		FVector ToTraceEnd = RandomEnd - TraceStart;
		HitTargets.Add(TraceStart + TRACE_LENGTH * ToTraceEnd / ToTraceEnd.Size());
	}
}
