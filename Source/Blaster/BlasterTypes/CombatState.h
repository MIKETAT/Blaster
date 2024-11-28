#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_UnOccupied UMETA(DisplayName = "UnOccupied"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),
	ECS_ThrowingGrenade UMETA(DisplayName = "ThrowingGrenade"),
	ECS_SwapWeapons UMETA(DisplayName = "SwapWeapons"),
	ECS_MAX UMETA(DisplayName = "Default MAX")
};