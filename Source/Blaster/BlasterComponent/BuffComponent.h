#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


class ABlasterCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	friend class ABlasterCharacter;
	void Heal(float HealAmount, float HealingTime);
	void ReplenishShield(float ShieldAmount, float ReplenishTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffJump(float JumpVelocity, float BuffTime);
	void SetInitialSpeed(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float Velocity);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);
private:
	UPROPERTY()
	ABlasterCharacter* Character;

	/**
	 * Heal Buff
	 */
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	/**
	 * Shiedd Buff
	 */
	bool bReplenishShield = false;
	float ReplenishShieldRate = 0.f;
	float AmountToReplenish = 0.f;
	
	/**
	 * Speed Buff
	 */
	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	void ResetSpeed();

	/**
	 * Jump Buff 
	 */
	FTimerHandle JumpBuffTimer;
	float InitialJumpVelocity;
	void ResetJumpVelocity();

	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);
};
