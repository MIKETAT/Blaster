// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

enum class EWeaponType : uint8;
class ABlasterPlayerController;
class ABlasterCharacter;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_Default_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	void ShowPickupWidget(bool bShowWidget);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	virtual void Fire(const FVector& HitTarget);
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterSpeed; }
	FORCEINLINE bool CanAutomaticFire() const { return bAutomaticFire; }
	FORCEINLINE bool CanFire() const{ return bCanFire && Ammo > 0; }	// 这里我把CanFire变量设置为Weapon的成员，感觉更合理
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE bool IsEmpty() const { return Ammo == 0; }
	FORCEINLINE bool IsFull() const { return Ammo == MaxCapacity; }
	FORCEINLINE bool IsDefaultWeapon () const { return bIsDefaultWeapon; }
	FORCEINLINE void SetIsDefaultWeapon(bool IsDefault) { bIsDefaultWeapon = IsDefault; }
	FORCEINLINE int32 GetMaxCapacity() const { return MaxCapacity; }
	FORCEINLINE void SetWeaponFireStatus(bool CanFire) { bCanFire = CanFire; }
	FORCEINLINE FTimerHandle& GetFireTimer() { return FireTimer; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	void Drop();
	void SetWeaponPhysicsAndCollision(bool bEnable);

	
	void SetWeaponState(EWeaponState state);
	FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
	void OnWeaponStateChange();
	void OnEquip();
	void OnDrop();
	void OnEquipSecondary();
	
	UFUNCTION()
	void OnRep_Ammo();
	virtual void OnRep_Owner() override;
	void SpendRound();
	void SetHUDAmmo();
	void AddAmmo(int AmmoAmount);

	/**
	 * Enable or Disable custom depth
	 */
	void EnableCustomDepth(bool bEnable);
	
	UPROPERTY()
	bool bCanFire = true;
protected:
	virtual void BeginPlay() override;

	UFUNCTION( )
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlapComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherIndex,
		bool mFromSweep,
		const FHitResult& SweepHitResult 
	);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlapComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherIndex
	);
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;
	
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 20.f;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickUpWidget;
	
	FTimerHandle FireTimer;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomaticFire = true;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;
	
	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(EditAnywhere, Category= "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	/**
	 *	Zoomed FOV while Aiming
	 */
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterSpeed = 20.f;
	
	UPROPERTY(EditAnywhere, ReplicatedUsing= OnRep_Ammo)
	int32 Ammo = 30;

	UPROPERTY(EditAnywhere)
	int32 MaxCapacity = 30;

	UPROPERTY()
	ABlasterCharacter* BlasterOwner;

	UPROPERTY()
	ABlasterPlayerController* BlasterController;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

// public variables 
public:
	/**
	 *  Texture for crosshairs
	 */
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairLeft;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairRight;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairTop;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairBottom;

	UPROPERTY(EditAnywhere)
	USoundCue* EquipSound;

	UPROPERTY(EditAnywhere)
	USoundCue* OutOfAmmoSound;

private:
	bool bIsDefaultWeapon = false;		// 如果是自带的武器，角色携带其死亡时销毁
};
