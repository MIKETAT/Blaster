// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/ActorComponent.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/Weapon.h"
#include "CombatComponent.generated.h"


enum class ECombatState : uint8;
enum class EWeaponType : uint8;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; }
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	void EquipWeapon(class AWeapon* WeaponToEquipped);
	void DropWeapon();
	void EquipPrimitiveWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);
	void DropCurrentWeaponAndEquipAnotherOne(AWeapon* WeaponToEquip);
	// CanSwapWeapon means we have two weapons
	FORCEINLINE bool CanSwapWeapon() const { return	Character && EquippedWeapon && SecondaryWeapon && EquippedWeapon->bCanFire; }; // 返回此时是否可以切换武器 即 EquippedWeapon SecondaryWeapon均不为空且OverlappingWeapon为空
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	void SwapWeapons();
	void TakeSecondaryWeapon();
	void PutSecondaryWeapon();
	
	bool CanFire() const;
	
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void FinishSwap();

	UFUNCTION(BlueprintCallable)
	void TestNotify();
	
	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();
	
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();
	
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void FireHitScanWeapon();
	void FireProjectileWeapon();
	void FireShotgun();
	
protected:
	virtual void BeginPlay() override;
	void SetAiming(bool isAiming);

	/**
	 * Fire
	 */
	void FireButtonPressed(bool bPressed);
	void Fire();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void ShotgunLocalFire(const TArray<FVector_NetQuantize>& HitTargets);

	UFUNCTION(Server, Reliable, WithValidation)
	void ShotgunServerFire(const TArray<FVector_NetQuantize>& HitTargets, const float FireDelay);
	
	
	void StartFireTimer();
	void FireTimerFinished();

	void Reload();
	void ThrowGrenade();
	void ShowGrenade(bool bShowGrenade);

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();
	
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool isAiming);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, const float FireDelay);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& HitTargets);
	
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;
	
private:
	UPROPERTY()
	ABlasterCharacter* Character;
	UPROPERTY()
	class ABlasterPlayerController* Controller;
	UPROPERTY()
	ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_TheFlag)
	AWeapon* TheFlag;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;
	
	UPROPERTY(ReplicatedUsing = OnRep_bIsAiming)
	bool bIsAiming = false;

	UPROPERTY(ReplicatedUsing = OnRep_bHoldingTheFlag)
	bool bHoldingTheFlag = false;
	
	bool bAimingButtonPressed = false;		// is Aiming Button  pressed now

	bool bLocallyReloading = false;			// same as bAimingButtonPressed. record whether we are reloading locally
	
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo);
	int32 CarriedAmmo = 30;	// 所携带的当前装备武器的子弹数量, 

	UPROPERTY(EditAnywhere)
	int32 MaxAmmoAmount = 500;


	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 5;
	
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = MaxGrenades;
	
	UPROPERTY(EditAnywhere)
	int32 InitialAmmoAmount = 30;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_UnOccupied;

	UFUNCTION()
	void OnRep_bIsAiming();

	UFUNCTION()
	void OnRep_bHoldingTheFlag();
	
	UFUNCTION()
	void OnRep_CarriedAmmo();

	UFUNCTION()
	void OnRep_Grenades();

	UFUNCTION()
	void OnRep_CombatState();
	
	UFUNCTION()
	void OnRep_EquippedWeapon(AWeapon* LastWeapon);

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	UFUNCTION()
	void OnRep_TheFlag();

	TMap<EWeaponType, int32> CarriedAmmoMap;
	/***
	 *	HUD and Crosshairs
	 */
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector HitTarget;
	FHUDPackage HUDPackage;

	/**
	 * Aiming FOV
	 */
	// Default filed of view; set to camera in BeginPlay
	float DefaultFOV;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);
	void InitWeaponAmmo();
	void SpawnDefaultWeapon();
	void DropEquippedWeapon();
	void DropOrDestroyWeapon(AWeapon* WeaponToHandle);
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachFlagToLeftHand(AWeapon* Flag);
	void AttachActorToBackpack(AActor* ActorToAttach);
	// update ammo and the hud
	void ReloadAmmo();
	void UpdateAmmoHUD();
	void UpdateGrenades();

	void HandleReload();
	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
	void AutoReloadEmptyWeapon();
	void UpdateShotgunAmmoValues();
	
	int32 AmountToReload();
};
