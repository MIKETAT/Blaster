#pragma once

UENUM(BlueprintType)
enum class ETeam : uint8
{
	ET_TeamRed UMETA(DisplayName = "TeamRed"),
	ET_TeamBlue UMETA(DisplayName = "TeamBlue"),
	ET_None UMETA(DisplayName = "None"),

	ET_MAX UMETA(DisplayName = "DefaultMax")
};