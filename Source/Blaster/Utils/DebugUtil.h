// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

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

	static void PrintMsg(const FString& Msg, const FColor& Color)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, Color, Msg, true, FVector2d(1.5f, 1.5f));
	}

	
	static void PrintMsg(const FText& Msg, const FColor& Color)
	{
		FString Message = Msg.ToString();
		PrintMsg(Message, Color);
	}

	
	static void PrintMsg(const FName& Msg, const FColor& Color)
	{
		FString Message = Msg.ToString();
		PrintMsg(Message, Color);
	}
};
