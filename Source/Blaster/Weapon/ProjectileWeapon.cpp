// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
#include "Blaster/Utils/DebugUtil.h"

/**
 * Server
 *		Use SSR
 *			LocallyControlled
 *
 *			Not LocallyControlled
 *		Don't use SSR
 *
 * Client
 *   	Use SSR
 *			LocallyControlled
 *
 *			Not LocallyControlled
 *		Don't use SSR
 *			LocallyControlled
 *
 *			
 *			Not LocallyControlled
 */
void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	DebugUtil::PrintMsg(TEXT("AProjectileWeapon Firing"));
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	// 获取枪口位置":
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();
	if (!MuzzleFlashSocket || !World || !InstigatorPawn || !ProjectileClass || !SSRProjectileClass)	return;
	
	FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());	// it needs the mesh that the socket belong to
	FVector ToTarget = HitTarget - SocketTransform.GetLocation();
	DrawDebugPoint(GetWorld(), SocketTransform.GetLocation(),5.f, FColor::Green, false, 3.f);
	FRotator TargetRotation = ToTarget.Rotation();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();	// projectile's owner is the weapon's owner, the character controlling it
	SpawnParameters.Instigator = InstigatorPawn;
	AProjectile* SpawnedProjectile = nullptr;
	
	if (bUseServerSideRewind)		// SSR
	{
		if (InstigatorPawn->HasAuthority())	// server
		{
			if (InstigatorPawn->IsLocallyControlled())		// server + SSR + locally controlled  -> Spawn Replicated projectile, No SSR
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
			} else					// server + SSR + not locally controlled -> SPawn No Replicated Projectile,  SSR
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(SSRProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
				SpawnedProjectile->bUseServerSideRewind = true;
			}
		} else	// client
		{
			if (InstigatorPawn->IsLocallyControlled())		// client + SSR + locally controlled -> Spawn no Replicated Projectile, Use SSR
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(SSRProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
				SpawnedProjectile->bUseServerSideRewind = true;
				SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
				SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				SpawnedProjectile->Damage = Damage;
			} else										// client + SSR + not locally controlled -> Spawn no replicated projectile, no SSR
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(SSRProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
				SpawnedProjectile->bUseServerSideRewind = false;
			}
		}
	} else							// Not use SSR, Spawn Replicated Projectile, No SSR
	{
		SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
		SpawnedProjectile->bUseServerSideRewind = false;
		SpawnedProjectile->Damage = Damage;
	}
}
