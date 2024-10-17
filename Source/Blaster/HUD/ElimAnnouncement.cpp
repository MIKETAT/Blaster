// Fill out your copyright notice in the Description page of Project Settings.


#include "ElimAnnouncement.h"

#include "Components/TextBlock.h"

void UElimAnnouncement::AddElimMessage(const FString& AttackerName, const FString& VictimName)
{
	FString ElimAnnouncementText = FString::Printf(TEXT("%s Killed %s"), *AttackerName, *VictimName);
	if (ElimMessage.Num() >= MaxElimMsg)
	{
		ElimMessage.RemoveAt(0);
	}
	ElimMessage.Add(ElimAnnouncementText);
	if (AnnouncementText)
	{
		AnnouncementText->SetText(GetElimAnnouncementText());
	}
}

void UElimAnnouncement::ClearElimMessage()
{
	if (!ElimMessage.IsEmpty())
	{
		ElimMessage.Empty();
	}
}

FText UElimAnnouncement::GetElimAnnouncementText() const
{
	FString Announcement;
	for (auto iter = ElimMessage.rbegin(); iter != ElimMessage.rend(); ++iter)
	{
		Announcement.Append(*iter + "\n");
	}
	
	UE_LOG(LogTemp, Error, TEXT("!!! = %s"), *Announcement)
	return FText::FromString(Announcement);
}
