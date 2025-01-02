// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "KismetTraceUtils.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Character/BlasterAnimInstance.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"
#include "Blaster/Weapon/Projectile.h"
#include "Blaster/Weapon/Shotgun.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 400.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, TheFlag);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
	DOREPLIFETIME(UCombatComponent, bHoldingTheFlag);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);	//只需要复制给持有者即可，无需复制给其他玩家，节省带宽
	DOREPLIFETIME_CONDITION(UCombatComponent, Grenades, COND_OwnerOnly);	
	// todo condition用法详解
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		InitWeaponAmmo();// todo 在client初始化可以吗，涉及到需要和server同步。因为这里本地的HUD显示弹药需要这个
	}
	SpawnDefaultWeapon();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		InterpFOV(DeltaTime);
		SetHUDCrosshairs(DeltaTime);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr || EquippedWeapon == nullptr)	return;
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD && EquippedWeapon)
		{
			HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
			HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
			HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
			HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
			HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
			// calc corsshair spread
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			// todo crouching crosshair spread
			FVector2D VelocityMultiplierRange(0.f, 1.f);	// [0, 1]
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
			if (Character->GetMovementComponent()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2, DeltaTime, 2.f);
			} else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0, DeltaTime, 30.f);
			}
			if (bIsAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.f);
			} else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}
			// always interp AimFactor to zero
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);
			HUDPackage.CrosshairSpread = CrosshairVelocityFactor +
									  CrosshairInAirFactor -
									  CrosshairAimFactor +
									  CrosshairShootingFactor + 0.5f;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::OnRep_bIsAiming()
{
	if (Character && Character->IsLocallyControlled())
	{
		// client本地是否isAim取决于当前AimButtonPressed是否为true,避免lag时同步导致的本地not pressed, 但是同步过来是Pressed状态
		// 也就是说不根据server 同步过来的值
		bIsAiming = bAimingButtonPressed;	
	}
}

void UCombatComponent::OnRep_bHoldingTheFlag()
{
	if (bHoldingTheFlag && Character && Character->IsLocallyControlled())
	{
		Character->Crouch();
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Controller) : Controller;
	if (Controller && EquippedWeapon)
	{
		Controller->SetHUDAmmo(EquippedWeapon->GetAmmo(), CarriedAmmo);
	}
	bool bJumpToShotgunEnd =
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CarriedAmmo == 0;
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateGrenades();
}

// on client
void UCombatComponent::OnRep_CombatState()
{
	DebugUtil::PrintMsg(GetOwner(), FString::Printf(TEXT("OnRep_CombatState")));
	switch (CombatState)
	{
		case ECombatState::ECS_UnOccupied:
			// 切回到UnOccupied，此时开火键按住的话就开火
			DebugUtil::PrintMsg(GetOwner(), FString::Printf(TEXT("OnRep_CombatState ECS_UnOccupied")));
			if (bFireButtonPressed)
			{
				Fire();
			}
			break;
		case ECombatState::ECS_Reloading:
			DebugUtil::PrintMsg(GetOwner(), FString::Printf(TEXT("OnRep_CombatState ECS_Reloading")));
			if (Character && !Character->IsLocallyControlled())
			{
				HandleReload();
			}
			break;
		case ECombatState::ECS_ThrowingGrenade:
			DebugUtil::PrintMsg(GetOwner(), FString::Printf(TEXT("OnRep_CombatState ECS_ThrowingGrenade")));
			if (Character && !Character->IsLocallyControlled())
			{
				DebugUtil::PrintMsg(Character, FString::Printf(TEXT("OnRep_CombatState ECS_ThrowingGrenade")));
				Character->PlayThrowGrenadeMontage();
				AttachActorToLeftHand(EquippedWeapon);
				ShowGrenade(true);
			}
			break;
		case ECombatState::ECS_SwapWeapons:
			DebugUtil::PrintMsg(GetOwner(), FString::Printf(TEXT("OnRep_CombatState ECS_SwapWeapons")));
			if (Character && !Character->IsLocallyControlled())		// local的立刻就播放montage了
			{
				Character->PlaySwapMontage();
			}
			break;
		default:
			break;
	}
}

/**
 *	Fire Section
 */
// 自动武器，bFireButtonPressed 会在一段时间内为true。 而repolicated 只会在变化时复制。因此无法连续播放动画
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	// out of ammo
	if (EquippedWeapon && EquippedWeapon->GetAmmo() == 0 && EquippedWeapon->OutOfAmmoSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->OutOfAmmoSound,
			EquippedWeapon->GetActorLocation());
	}
	if (!CanFire())		return;
	// 开火
	EquippedWeapon->SetWeaponFireStatus(false);	// 开火执行时不能同时开火
	switch (EquippedWeapon->FireType)
	{
		case EFireType::EFT_Projectile:
			FireProjectileWeapon();
			break;
		case EFireType::EFT_HitScan:
			FireHitScanWeapon();
			break;
		case EFireType::EFT_Shotgun:
			FireShotgun();
			break;
		default:
			break;
	}
	CrosshairShootingFactor = FMath::Max(CrosshairShootingFactor + 0.2f, 1.f);
	StartFireTimer();
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr)	return;
	// 霰弹枪装弹时可打断
	if (Character && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_UnOccupied;
		return;
	}
	if (Character && CombatState == ECombatState::ECS_UnOccupied)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}


void UCombatComponent::ShotgunLocalFire(const TArray<FVector_NetQuantize>& HitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (!Shotgun || !Character)	return;
	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_UnOccupied)
	{
		Character->PlayFireMontage(bIsAiming);
		Shotgun->FireShotgun(HitTargets);
		CombatState = ECombatState::ECS_UnOccupied;
	}
}

void UCombatComponent::ShotgunServerFire_Implementation(const TArray<FVector_NetQuantize>& HitTargets, const float FireDelay)
{
	MulticastShotgunFire(HitTargets);
}

bool UCombatComponent::ShotgunServerFire_Validate(const TArray<FVector_NetQuantize>& HitTargets, const float FireDelay)
{
	if (EquippedWeapon)
	{
		return FMath::IsNearlyEqual(EquippedWeapon->GetFireDelay(), FireDelay);
	}
	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& HitTargets)
{
	// 客户端本地自己操控的角色已经调用了LocalFire, 不必再通过网络同步调用一次LocalFire
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority())	return;
	ShotgunLocalFire(HitTargets);
}

void UCombatComponent::StartFireTimer()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return;
	Character->GetWorldTimerManager().SetTimer(
		EquippedWeapon->GetFireTimer(),
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->GetFireDelay());
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr)	return;
	EquippedWeapon->SetWeaponFireStatus(true);
	// 如果是自动武器且仍在开火
	if (EquippedWeapon->CanAutomaticFire() && bFireButtonPressed)
	{
		Fire();
	}
	AutoReloadEmptyWeapon();
}

// 更新子弹HUD. 
void UCombatComponent::UpdateAmmoHUD()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Controller) : Controller;
	if (Controller == nullptr)	return;
	if (EquippedWeapon == nullptr || !CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		Controller->SetHUDAmmo(0, 0);
	} else
	{
		//CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		Controller->SetHUDAmmo(EquippedWeapon->GetAmmo(), CarriedAmmo);
	}
}

// on server
// reload the ammo
void UCombatComponent::ReloadAmmo()
{
	if (Character == nullptr || EquippedWeapon == nullptr || CarriedAmmo <= 0)	return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo -= ReloadAmount;
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] = CarriedAmmo;
		EquippedWeapon->AddAmmo(ReloadAmount);
		UpdateAmmoHUD();
	}
}


void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	/*if (GEngine)
	{
		const UEnum* EnumObj = FindObject<UEnum>(ANY_PACKAGE, TEXT("EWeaponType"), true);
		FString WeaponTypeStr = *EnumObj->GetDisplayNameTextByIndex(static_cast<uint8>(WeaponType)).ToString();
		FString Info = FString::Printf(TEXT("Picked %s Ammo %d"), *WeaponTypeStr, AmmoAmount);
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, Info, true, FVector2D(4.f, 4.f));
	}*/
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxAmmoAmount);
		if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType)
		{
			CarriedAmmo = CarriedAmmoMap[WeaponType];
		}
		UpdateAmmoHUD();
	}
	if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::UpdateGrenades()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	UE_LOG(LogTemp, Error, TEXT("Update Shotgun Ammo"));
	if (Character == nullptr || EquippedWeapon == nullptr)	return;
	for (const auto& it : CarriedAmmoMap)
	{
		//UE_LOG(LogTemp, Error, TEXT("WeaponType = %d, Ammo = %d"), it.Key, it.Value);
	}
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo -= 1;
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDAmmo(EquippedWeapon->GetAmmo(), CarriedAmmo);
		}
		EquippedWeapon->AddAmmo(1);
		EquippedWeapon->bCanFire = true;
	}
	// 检查是否装满 or 没多余子弹了
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return 0;
	int32 RoomForReload = EquippedWeapon->GetMaxCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 CarriedWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(CarriedWeaponAmmo, RoomForReload);
		return FMath::Clamp(RoomForReload, 0, Least);	
	}
	return 0;
}

void UCombatComponent::Reload()
{
	if (EquippedWeapon == nullptr || EquippedWeapon->IsFull())	return;
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_UnOccupied && !bLocallyReloading)
	{
		ServerReload();
		HandleReload();
		bLocallyReloading = true;
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return;
	CombatState = ECombatState::ECS_Reloading;
	if (!Character->IsLocallyControlled())		// todo OnRep里还能理解，这里的含义是client上非本地控制的意思还是server上的？
	{
		HandleReload();
	}
}

// todo reload的动画需要改，只上半身reload，不影响下半身，不会改变状态(蹲下->站起)这种
void UCombatComponent::HandleReload()
{
	CombatState = ECombatState::ECS_Reloading;
	if (Character)
	{
		Character->PlayReloadMontage();	
	}
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr)	return;
	bLocallyReloading = false;
	// 重要变量的变化，例如状态的变化都在server执行
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_UnOccupied;
		ReloadAmmo();
	}
	// 换弹完成，仍按住了开火
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::FinishSwap()
{
	DebugUtil::PrintMsg(Character, TEXT("Call FinishSwap"));
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_UnOccupied;
	}
	if (Character)
	{
		Character->bFinishSwapping = true;
	}
	if (SecondaryWeapon)
	{
		SecondaryWeapon->EnableCustomDepth(true);	// todo
	}
}

void UCombatComponent::TestNotify()
{
	DebugUtil::PrintMsg(Character, FString::Printf(TEXT("TestNotify")));
}

void UCombatComponent::FinishSwapAttachWeapons()
{
	// local effects
	PlayEquipWeaponSound(EquippedWeapon);
	
	FString Name = Character ? Character->GetName() : FString("No Name");
	DebugUtil::PrintMsg(Character, FString::Printf(TEXT("%s Call FinishSwapAttachWeapons"), *Name));
	if (Character == nullptr || !EquippedWeapon|| !Character->HasAuthority())	return;		// 避免client重复swap
	AWeapon* TempWeapon = EquippedWeapon;
	// Update CarriedAmmo
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] = CarriedAmmo;
	}
	EquippedWeapon = SecondaryWeapon;
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	SecondaryWeapon = TempWeapon;
	AttachActorToBackpack(SecondaryWeapon);
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

	AutoReloadEmptyWeapon();
	if (Character->IsLocallyControlled() && HUD)
	{
		HUD->SetHUDCrosshairEnabled(EquippedWeapon != nullptr);
		UpdateAmmoHUD();
	}
	DebugUtil::PrintMsg(Character, FString::Printf(TEXT("FinishSwapAttachWeapons")));
	DebugUtil::PrintMsg(Character, FString("Now First Weapon is " + EquippedWeapon->GetName()));
	DebugUtil::PrintMsg(Character, FString("Now Second Weapon is " + SecondaryWeapon->GetName()));
}

void UCombatComponent::ThrowGrenade()
{
	if (Grenades == 0)	return;
	
	if (CombatState != ECombatState::ECS_UnOccupied || !Character || !EquippedWeapon)	return;

	DebugUtil::PrintMsg(Character, FString::Printf(TEXT("ThrowGrenade")));

	CombatState = ECombatState::ECS_ThrowingGrenade;
	AttachActorToLeftHand(EquippedWeapon);
	ShowGrenade(true);
	Character->PlayThrowGrenadeMontage();
	if (Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateGrenades();
	} else
	{
		ServerThrowGrenade();
	}
}

void UCombatComponent::ShowGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}


void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades == 0)	return;

	DebugUtil::PrintMsg(Character, FString::Printf(TEXT("ServerThrowGrenade_Implementation")));
	
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		DebugUtil::PrintMsg(Character, FString::Printf(TEXT("Call Play Montage ServerThrowGrenade_Implementation")));
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon); // save?
		ShowGrenade(true);
	}
	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateGrenades();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, const float FireDelay)
{
	MulticastFire(TraceHitTarget);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, const float FireDelay)
{
	if (EquippedWeapon)
	{
		return FMath::IsNearlyEqual(EquippedWeapon->GetFireDelay(), FireDelay);
	}
	return true;
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 客户端本地自己操控的角色已经调用了LocalFire, 不必再通过网络同步调用一次LocalFire
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority())	return;
	LocalFire(TraceHitTarget);
}

/**
 * Aiming Section
 */
void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	// trace from center of the screen
	FVector2d ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	FVector2d CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);	// screen space
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	// for each machine, player 0 is the player that's playing the game
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		//Controller,
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition, CrosshairWorldDirection);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		// linetrace should begin in front of character, avoid shooting behind the character
		float CameraDistance = (Start - Character->GetActorLocation()).Size();
		Start += CrosshairWorldDirection * CameraDistance;
		
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		FCollisionQueryParams Params;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		// todo 这里射线检测添加了IgnoreActor, 但是Pitch过大会使动画不正确，考虑限制contorller的rotation
		bool isHit = GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility,
			QueryParams
			);
		// todo 想想这里未击中的话如何处理，让人物持枪动画正常
		// 未击中，设置HitResult的ImpactPoint
		if (!isHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>()
			&& TraceHitResult.GetActor() != Character)
		{
			DrawDebugPoint(GetWorld(), TraceHitResult.ImpactPoint, 5.f, FColor::Orange);
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		} else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetAiming(bool isAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr)		return;
	bIsAiming = isAiming;	// set before rpc and replicated
	ServerSetAiming(bIsAiming);
	Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		Controller->SetHUDSniperScope(bIsAiming);
	}
	if (Character->IsLocallyControlled())
	{
		bAimingButtonPressed = isAiming;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool isAiming)
{
	bIsAiming = isAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (Character == nullptr)	return;
	if (bIsAiming && EquippedWeapon)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	} else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed); // UnZoomSpeed 各个武器一样
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

/**
 * NetWork
 */
void UCombatComponent::OnRep_EquippedWeapon(AWeapon* LastWeapon)	// todo LastWeapon是否保留
{
	if (Character == nullptr)	return;
	
	if (EquippedWeapon)
	{
		// test
		/*if (EquippedWeapon != LastWeapon)
		{
			DebugUtil::PrintMsg(Character, FString::Printf(TEXT("OnRep_EquippedWeapon EquippedWeapon != LastWeapon")));
		}
		if (LastWeapon)
		{
			DebugUtil::PrintMsg(Character, FString::Printf(TEXT("OnRep_EquippedWeapon  LastWeapon: %s"), *LastWeapon->GetWeaponName().ToString()));
		}*/
		// TODO 为什么下面这段不加上，client依然可以执行捡起武器。
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;	// 禁用面向移动方向
		Character->bUseControllerRotationYaw = true;	// Pawn的yaw等于controller的yaw

		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDAmmo(EquippedWeapon->GetAmmo(), CarriedAmmo);
		}
		PlayEquipWeaponSound(EquippedWeapon);
		//DebugUtil::PrintMsg(Character, FString::Printf(TEXT("OnRep_EquippedWeapon: %s"), *EquippedWeapon->GetWeaponName().ToString()));
	}
	if (Character->IsLocallyControlled() && HUD)
	{
		HUD->SetHUDCrosshairEnabled(EquippedWeapon != nullptr);
		UpdateAmmoHUD();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (Character && SecondaryWeapon)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquipWeaponSound(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;	// 禁用面向移动方向
		Character->bUseControllerRotationYaw = true;	// Pawn的yaw等于controller的yaw
		if (SecondaryWeapon && SecondaryWeapon->GetWeaponMesh())
		{
			SecondaryWeapon->GetWeaponMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
			SecondaryWeapon->GetWeaponMesh()->MarkRenderStateDirty();
		}
		DebugUtil::PrintMsg(Character, FString::Printf(TEXT("OnRep_SecondaryWeapon: %s"), *SecondaryWeapon->GetWeaponName().ToString()));
	}
}

void UCombatComponent::OnRep_TheFlag()
{
	if (TheFlag)
	{
		TheFlag->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachFlagToLeftHand(TheFlag);
	}
}

// 初始化CarriedAmmoMap
void UCombatComponent::InitWeaponAmmo()
{
	const UEnum* EnumObj = FindObject<UEnum>(ANY_PACKAGE, TEXT("EWeaponType"), true);
	
	UEnum* MyEnum = StaticEnum<EWeaponType>();
	for (int i = 0; i < MyEnum->NumEnums()-1; i++)
	{
		EWeaponType type = EWeaponType(MyEnum->GetValueByIndex(i));
		CarriedAmmoMap.Emplace(type, 0);
	}
}

void UCombatComponent::SpawnDefaultWeapon()
{
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if (!BlasterGameMode || !World || !Character || !Character->GetCombat())	return;
	AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
	EquipWeapon(DefaultWeapon);
	if (DefaultWeapon)
	{
		DefaultWeapon->SetIsDefaultWeapon(true);
	}
	//DebugUtil::PrintMsg(Character, TEXT("SpawnDefaultWeapon"));
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Drop();	// 已有武器则扔掉替换
		EquippedWeapon = nullptr;
	}
	if (Character->IsLocallyControlled() && HUD)
	{
		HUD->SetHUDCrosshairEnabled(EquippedWeapon != nullptr);
		UpdateAmmoHUD();
	}
}

void UCombatComponent::DropOrDestroyWeapon(AWeapon* WeaponToHandle)
{
	if (WeaponToHandle == nullptr)	return;
	if (WeaponToHandle->IsDefaultWeapon())
	{
		WeaponToHandle->Destroy();	// 出生自带武器销毁
	} else {
		WeaponToHandle->Drop();
	}
	UpdateAmmoHUD();
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr)	return;
	// todo 为什么这里不在client执行AttachActor，client照样能完成捡起武器
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}


void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr)	return;
	// bUsePistolSocket
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("LeftHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachFlagToLeftHand(AWeapon* Flag)
{
	DebugUtil::PrintMsg(Character, TEXT("AttachFlagToLeftHand"));
	if (Character == nullptr || Character->GetMesh() == nullptr || Flag == nullptr)	return;
	const USkeletalMeshSocket* FlagSocket = Character->GetMesh()->GetSocketByName(FName("FlagSocket"));
	if (FlagSocket)
	{
		FlagSocket->AttachActor(Flag, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr)	return;
	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	if (BackpackSocket)
	{
		BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->EquipSound,
			WeaponToEquip->GetActorLocation()
		);
	}
}

void UCombatComponent::AutoReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

// only on server
/**
 * 1. no weapon
 * 2. already have one
 * 3. have two weapons
 */
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquipped)
{
	if (!Character || !WeaponToEquipped || CombatState != ECombatState::ECS_UnOccupied)	return;
	// capture the flag
	if (WeaponToEquipped->GetWeaponType() == EWeaponType::EWT_Flag)
	{
		if (Character)		Character->Crouch();
		bHoldingTheFlag = true;
		WeaponToEquipped->SetWeaponState(EWeaponState::EWS_Equipped);
		WeaponToEquipped->SetOwner(Character);
		AttachFlagToLeftHand(WeaponToEquipped);
		TheFlag = WeaponToEquipped;
		return;
	}
	// equip weapon
	if (EquippedWeapon == nullptr)
	{
		EquipPrimitiveWeapon(WeaponToEquipped);
	} else if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(WeaponToEquipped);
	} else
	{
		DropCurrentWeaponAndEquipAnotherOne(WeaponToEquipped);
	}
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;	// 禁用面向移动方向
	Character->bUseControllerRotationYaw = true;	// Pawn的yaw等于controller的yaw
	UpdateAmmoHUD();
}

void UCombatComponent::DropWeapon()
{
	if (!EquippedWeapon)	return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] = CarriedAmmo;
		CarriedAmmo = 0;
	}
	EquippedWeapon->Drop();
	EquippedWeapon = nullptr;
	if (Character->IsLocallyControlled() && HUD)
	{
		HUD->SetHUDCrosshairEnabled(EquippedWeapon != nullptr);
		UpdateAmmoHUD();
	}
}

void UCombatComponent::EquipPrimitiveWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr)	return;
	//DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->ShowPickupWidget(false);
	// 更新CarriedAmmo
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	} else
	{
		CarriedAmmo = 0;
	}
	PlayEquipWeaponSound(EquippedWeapon);
	AutoReloadEmptyWeapon();
	if (Character->IsLocallyControlled() && HUD)
	{
		HUD->SetHUDCrosshairEnabled(EquippedWeapon != nullptr);
		UpdateAmmoHUD();
	}
	//DebugUtil::PrintMsg(Character, TEXT("Equip Primitive Weapon"));
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr)	return;
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
	PlayEquipWeaponSound(SecondaryWeapon);
	SecondaryWeapon->SetOwner(Character);
	SecondaryWeapon->ShowPickupWidget(false);
	if (SecondaryWeapon->GetWeaponMesh())
	{
		SecondaryWeapon->GetWeaponMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		SecondaryWeapon->GetWeaponMesh()->MarkRenderStateDirty();	// todo 研究一下
	}
	//DebugUtil::PrintMsg(Character, TEXT("Equip Secondary Weapon"));
}

void UCombatComponent::DropCurrentWeaponAndEquipAnotherOne(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr || EquippedWeapon == nullptr)	return;
	DropWeapon();
	EquipPrimitiveWeapon(WeaponToEquip);
}

void UCombatComponent::SwapWeapons()
{
	if (!Character || !EquippedWeapon || !SecondaryWeapon || CombatState != ECombatState::ECS_UnOccupied)		return;
	Character->PlaySwapMontage();
	
	CombatState = ECombatState::ECS_SwapWeapons;
	Character->bFinishSwapping = false;
	DebugUtil::PrintMsg(Character, TEXT("Call SwapWeapons"));
}

// 无枪时掏出第二把枪
void UCombatComponent::TakeSecondaryWeapon()
{
	DebugUtil::PrintMsg(Character, TEXT("TakeSecondaryWeapon"));
	if (EquippedWeapon != nullptr || SecondaryWeapon == nullptr)	return;
	AWeapon* TempWeapon = SecondaryWeapon;
	SecondaryWeapon->Drop();
	EquipPrimitiveWeapon(TempWeapon);
	SecondaryWeapon = nullptr;
}

void UCombatComponent::PutSecondaryWeapon()
{
	DebugUtil::PrintMsg(Character, TEXT("PutSecondaryWeapon"));
	if (EquippedWeapon == nullptr || SecondaryWeapon != nullptr)	return;
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon->Drop();
	EquipSecondaryWeapon(TempWeapon);
	EquippedWeapon = nullptr;
	if (Character->IsLocallyControlled() && HUD)
	{
		HUD->SetHUDCrosshairEnabled(EquippedWeapon != nullptr);
		UpdateAmmoHUD();
	}
}

bool UCombatComponent::CanFire() const
{
	// 无武器 || 武器无法开火(此刻不能开火/没子弹) || 当前状态不能开火
	if (EquippedWeapon == nullptr || !EquippedWeapon->CanShoot())
	{
		return false;
	}
	// shotgun在换弹时也可开火
	if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun && CombatState == ECombatState::ECS_Reloading)	return true;
	if (bLocallyReloading)		return false;
	return CombatState == ECombatState::ECS_UnOccupied;
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();	
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	// Jump to ShotgunEnd
	if (Character == nullptr || Character->GetMesh() == nullptr)	return;
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}	
}

void UCombatComponent::ThrowGrenadeFinished()
{
	DebugUtil::PrintMsg(Character, FString::Printf(TEXT("ThrowGrenadeFinished")));
	CombatState = ECombatState::ECS_UnOccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowGrenade(false);
	FString Name = Character ? Character->GetName() : "No Name";
	DebugUtil::PrintMsg(Character, FString::Printf(TEXT("LaunchGrenade Name is %s"), *Name));
	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority())		// server直接调用ServerFire就行
		{
			LocalFire(HitTarget);	
		}
		ServerFire(HitTarget, EquippedWeapon->GetFireDelay());
	}
}

// for assault rifle
void UCombatComponent::FireProjectileWeapon()
{
	if (Character && EquippedWeapon)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority())
		{
			LocalFire(HitTarget);
		}	
		ServerFire(HitTarget, EquippedWeapon->GetFireDelay());
	}
}

// Calc hit results locally
void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (!Shotgun)	return;
	TArray<FVector_NetQuantize> HitTargets;
	Shotgun->ShotgunTraceWithScatter(HitTarget, HitTargets);
	if (!Character->HasAuthority())
	{
		ShotgunLocalFire(HitTargets);
	}
	ShotgunServerFire(HitTargets, EquippedWeapon->GetFireDelay());
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && Character->GetAttachedGrenade() && GrenadeClass)
	{
		const FVector LaunchLoaction = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - LaunchLoaction;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Character;
		SpawnParameters.Instigator = Character;
		
		if (GetWorld())
		{
			DebugUtil::PrintMsg(FString::Printf(TEXT("Server Spawn Grenade Location is %s"), *LaunchLoaction.ToString()));
			GetWorld()->SpawnActor<AProjectile>(
				GrenadeClass,
				LaunchLoaction,
				ToTarget.Rotation(),
				SpawnParameters);
		}
	}
}
