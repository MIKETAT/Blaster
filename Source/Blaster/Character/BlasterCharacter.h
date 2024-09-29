// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/InteractWithCrosshairsInterface.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

class UBuffComponent;
class UCombatComponent;
class ABlasterPlayerState;
enum class ECombatState : uint8;
class UTimelineComponent;
class ABlasterGameMode;
class UCameraComponent;
class AWeapon;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

// Functions
public:
	ABlasterCharacter();
	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void SetOverlappingWeapon(AWeapon* OverlapWeapon);
	bool isWeaponEquipped() const;
	bool isAiming() const;
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon() const;
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	/* 由于Health已经是Replicated的(改变会自动调用OnRep_Health, 利用这点比发rpc更节省资源)
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();*/
	FORCEINLINE ABlasterPlayerController* GetBlasterPlayerController() const
	{
		return BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	}

	FORCEINLINE ABlasterGameMode* GetBlasterGameMode() const
	{
		return BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	}

	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE ABlasterPlayerState* GetBlasterPlayerState() const { return BlasterPlayerState; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	ECombatState GetCombatState() const;
	
	float CalculateSpeed();
	void UpdateHealthHUD();
	void UpdateShieldHUD();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	// Only on Server
	void Elim();

	// 处理初始化相关的逻辑，
	void PollInit();

	//UFUNCTION(BlueprintImplementableEvent)
	//void ShowSniperScopeWidget(bool bShowScope);
	
protected:
	virtual void BeginPlay() override;
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void DropButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void ReloadButtomPressed();
	void GrenadeButtonPressed();
	void CalculateAO_Pitch();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	void TurnInPlace(float DeltaTime);
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
	// tood damage delegates in unreal
	// we can bind function which has signature below to delegate function like onTakenAnyDamage
	UFUNCTION()			// todo find out why it has to be UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "ttrue"))
	class UCombatComponent* Combat;
	
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "ttrue"))
	class UBuffComponent* Buff;

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAnimRotation;

	ETurningInPlace TurningInPlace;
	
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	/**
	 * Montages
	 */
	UPROPERTY(EditAnywhere, Category= Combat)
	UAnimMontage* FireWeaponMontage;
	
	UPROPERTY(EditAnywhere, Category= Combat)
	UAnimMontage* HitReactMontage;
	
	UPROPERTY(EditAnywhere, Category= Combat)
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category= Combat)
	UAnimMontage* ReloadMontage;
	
	UPROPERTY(EditAnywhere, Category= Combat)
	UAnimMontage* ThrowGrenadeMontage;
	
	bool bRotateRootBone;
	// 超过这个阈值，在SimProxy执行turn animation
	float TurnThreshold = 0.5f;
	float ProxyYaw;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyCurrentRotation;
	float TimeSinceLastReplication = 0.f;
	
	/**
	 * Player Health
	 */
	UPROPERTY(EditAnywhere, Category = "Player Status")
	float MaxHealth = 100;

	// Heath不放在 PlayerStats是因为其更新频率不够
	UPROPERTY(ReplicatedUsing= OnRep_Health, VisibleAnywhere, Category = "Player Status")
	float Health = MaxHealth;

	/**
	 * Player Shield
	 */

	UPROPERTY(EditAnywhere, Category = "Player Status0")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing= OnRep_Shield, EditAnywhere, Category= "Player Status")
	float Shield = 0.f;
	
	bool bElimmed = false;

	ABlasterPlayerController* BlasterPlayerController;
	class ABlasterGameMode* BlasterGameMode;

	FTimerHandle ElimTimer;
	
	UPROPERTY(EditDefaultsOnly)	// shouldn't have different elim timer
	float ElimDelay = 3.f;

	/**
	*	Dissolve Effects 
	*/
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	// Dynamic Instance we can change at runtime
	UPROPERTY(VisibleAnywhere, Category= Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material Instance set on bp, used with dynamic material instance 
	UPROPERTY(EditAnywhere, Category= Elim)
	UMaterialInstance* DissolveMaterialInstance;
	
	/**
	 * Elim Bot
	 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;
	
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ElimBotSound;

	class ABlasterPlayerState* BlasterPlayerState;
	
	/**
	 * Grenade
	 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;
	
	UFUNCTION()		// bind to track, so it needs to be UFUNCTION
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();
	
	void ElimTimerFinish();
	
	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UFUNCTION()
	void OnRep_Shield(float LastShield);
	
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastOverlapWeapon);

	void OnRep_ReplicatedMovement() override;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	void SetCharacterVisibility(bool bHide);

	void HideCharacterIfCameraClose();
};
