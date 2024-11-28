// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

/**
 * 
 */
class BLASTER_API DebugUtil
{
public:
	DebugUtil();
	~DebugUtil();

	static void LogMsg(const FString& Msg)
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
	}

	
	static void LogMsg(const FText& Msg)
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg.ToString());
	}

	
	static void LogMsg(const FName& Msg)
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Msg.ToString());
	}

	static void PrintMsg(ACharacter* Character, FString Msg)
	{
		PrintMsg(Character, *Msg);
	}
	
	static void PrintMsg(ACharacter* Character, const wchar_t* Msg)
	{
		if (Character == nullptr)
		{
			PrintMsg(Msg);
		} else
		{
			if (Character->HasAuthority())
			{
				PrintMsg(FString::Printf(TEXT("Server: %s"), Msg));
			} else
			{
				PrintMsg(FString::Printf(TEXT("Client: %s"), Msg));
			}
		}
	}
	
	static void PrintMsg(const wchar_t* Msg)
	{
		PrintMsg(FString::Printf(TEXT("%s"), Msg));
	}
	
	static void PrintMsg(const FString& Msg, const FColor& Color = FColor::Red)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, Color, Msg, true, FVector2d(1.5f, 1.5f));
		LogMsg(Msg);
	}
	
	
	static void PrintMsg(const FText& Msg, const FColor& Color = FColor::Red)
	{
		FString Message = Msg.ToString();
		PrintMsg(Message, Color);
	}

	
	static void PrintMsg(const FName& Msg, const FColor& Color = FColor::Red)
	{
		FString Message = Msg.ToString();
		PrintMsg(Message, Color);
	}
};
