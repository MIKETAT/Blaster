#pragma once

#include "CoreMinimal.h"
#include "Blaster/InteractWithCrosshairsInterface.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

class UBuffComponent;
class UCombatComponent;
enum class ECombatState : uint8;
class UTimelineComponent;
class ABlasterGameMode;
class UCameraComponent;
class AWeapon;
class ABlasterPlayerState;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	void ConstructHitBox();
	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void SetOverlappingWeapon(AWeapon* OverlapWeapon);
	void SetHoldingTheFlag(bool bHolding);
	bool isWeaponEquipped() const;
	bool isAiming() const;
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon() const;
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

	// Play Montages
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayHitReactMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();
	
	FVector GetHitTarget() const;
	FORCEINLINE AWeapon* GetOverlappingWeapon() const { return OverlappingWeapon; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
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
	FORCEINLINE ABlasterPlayerState* GetBlasterPlayerState() const { return BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UCombatComponent* GetCombat() const { return BlasterCombatComp; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	bool IsHoldingTheFlag() const;
	FORCEINLINE class ULagCompensationComponent* GetLagCompensationComponent() const { return LagCompensation; }
	bool IsLocallyReloading();
	ECombatState GetCombatState() const;
	ETeam GetTeam();
	
	float CalculateSpeed();
	void UpdateHealthHUD();
	void UpdateShieldHUD();
	void DropOrDestroyWeapons();

	// Only on Server
	void Elim(bool bPlayerLeftGame);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainTheLead();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();
	
	// 处理初始化相关的逻辑，
	void PollInit();
	void OnPlayerStateInitialized();
	void SetSpawnPoint();
	void SetTeamColor(ETeam TeamToSet);
	

	//UFUNCTION(BlueprintImplementableEvent)
	//void ShowSniperScopeWidget(bool bShowScope);

	// Press Quit Button
	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();
	
	FOnLeftGame OnLeftGame;
	
	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;

	bool bFinishSwapping = true;
	
protected:
	virtual void BeginPlay() override;
	void RotateInPlace(float DeltaTime);
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	virtual void Jump() override;
	void CrouchButtonPressed();

	void CameraButtonPressed();
	void CameraButtonReleased();
	
	// 下面的功能都转发到CombatComponent中执行
	void EquipButtonPressed();
	void DropButtonPressed();
	void FireButtonPressed();
	void FireButtonReleased();
	void AimButtonPressed();
	void AimButtonReleased();
	void ReloadButtonPressed();
	void GrenadeButtonPressed();

	// 
	void CalculateAO_Pitch();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	void TurnInPlace(float DeltaTime);

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;
	
	/**
	 *	Hit Box 
	 */
	UPROPERTY(EditAnywhere)
	class UBoxComponent* head;

	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;

	UPROPERTY(EditAnywhere)
	UBoxComponent* blanket;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* BlasterCombatComp;
	
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	class UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	class ULagCompensationComponent* LagCompensation;

	float AO_Yaw;
	float AO_Pitch;
	float InterpAO_Yaw;
	
	FRotator StartingAnimRotation;	// 上次运动时的Rotation

	ETurningInPlace TurningInPlace;
	
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;	// 距离墙体太近则隐藏Character

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

	UPROPERTY(EditAnywhere, Category= Combat)
	UAnimMontage* SwapMontage;
	
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

	bool bLeftGame = false;

	UPROPERTY()
	ABlasterPlayerController* BlasterPlayerController;	// 避免每次都cast

	UPROPERTY()
	ABlasterGameMode* BlasterGameMode;	// 避免每次都cast
	
	FTimerHandle ElimTimer;
	
	UPROPERTY(EditDefaultsOnly)	// shouldn't have different elim timer
	float ElimDelay = 10.f;

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
	 * Team Color
	 */
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* RedDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* BlueDissolveMatInst;

	// Color
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* RedMaterial;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* BlueMaterial;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* OriginalMaterial;
	
	/**
	 * Elim Bot
	 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;
	
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotEffectComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ElimBotSound;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;

	UPROPERTY()
	class UNiagaraComponent* CrownComponent;
	
	UPROPERTY()
	ABlasterPlayerState* BlasterPlayerState;
	
	//Grenade 
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

	UFUNCTION(Server, Reliable)
	void ServerDropButtonPressed();

	void SetCharacterVisibility(bool bHide);

	void HideCharacterIfCameraClose();
};
