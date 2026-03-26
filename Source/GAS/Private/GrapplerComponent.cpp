// Fill out your copyright notice in the Description page of Project Settings.
#include "GrapplerComponent.h"
#include "GrappleSocket.h"
#include "Kismet/GameplayStatics.h"
#include "MoverPawn.h"

UE_DEFINE_GAMEPLAY_TAG(Mover_IsGrappling, "Mover.IsGrappling");

// Sets default values for this component's properties
UGrapplerComponent::UGrapplerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	 PrimaryComponentTick.bCanEverTick = true;
}

bool UGrapplerComponent::TryToAttachToGrappleSocket(AMoverPawn* InPawn)
{
	//Checks if pawn is able to grapple onto a Grapple Socket
	if (!IsValid(CurrentGrappleSocket))
	{
		CurrentGrappleSocket = FindClosestGrappleSocket(InPawn);
		if (!IsValid(CurrentGrappleSocket)) return false;	
	}

	//Attach the pawn to the Grapple Socket
	CurrentGrappleSocket->AttachToGrappleSocket(InPawn);

	//Nothing went wrong, and it is safe to continue with animations or set other variables, etc.
	return true;
}

void UGrapplerComponent::DetachFromGrappleSocket(AMoverPawn* InPawn)
{
	//If pawn is attached to a Grapple Socket, deattach from it
	if (IsValid(CurrentGrappleSocket))
	{
		//Deattach the pawn from the Grapple Socket
		CurrentGrappleSocket->DetachFromGrappleSocket(InPawn, true);
	}
}

AGrappleSocket* UGrapplerComponent::FindClosestGrappleSocket(const AMoverPawn* InPawn) const
{
	if (!IsValid(InPawn)) return nullptr;

	//Initialize function-based variables
	AGrappleSocket* ClosestGrappleSocket = nullptr;
	float ClosestGrappleSocketDistance = FLT_MAX;

	//Gets all the Grapple Sockets in the world
	TArray<AActor*> FoundGrapples;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrappleSocket::StaticClass(), FoundGrapples);

	//Iterate through all the found Grapple Sockets to find the closest one and if pawn is in range to grapple
	for (AActor* GrappleSocketActor : FoundGrapples)
	{
		//Convert the Actor to a Grapple Socket
		AGrappleSocket* GrappleSocket = Cast<AGrappleSocket>(GrappleSocketActor);
		if (!IsValid(GrappleSocket)) continue;

		//Checks distance between the pawn and the Grapple Socket, and if pawn is in range to grapple
		//If the Grapple Socket is closer to the pawn than the current closest Grapple Socket, take its place as the closest 
		const float DistanceToPawn = GrappleSocket->GetDistanceFromAPawn(InPawn);
		if (DistanceToPawn < ClosestGrappleSocketDistance && GrappleSocket->IsInRangeToGrapple(InPawn))
		{
			ClosestGrappleSocket = GrappleSocket;
			ClosestGrappleSocketDistance = DistanceToPawn;
		}
	}

	//If there was no Grapple Sockets in the world or pawn was not in range to grapple onto a Grapple Socket, return nothing
	if (!IsValid(ClosestGrappleSocket))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:FindClosestGrappleSocket - ClosestGrappleSocket not valid or not any Grapple Sockets in world"), *StaticClass()->GetName());
		return nullptr;
	}

	//Return the closest Grapple Socket to the pawn
	return ClosestGrappleSocket;
}