// Fill out your copyright notice in the Description page of Project Settings.
#include "GrapplerComponent.h"
#include "GrappleSocket.h"
#include "Kismet/GameplayStatics.h"
#include "MoverPawn.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"

UE_DEFINE_GAMEPLAY_TAG(Mover_IsGrappling, "Mover.IsGrappling");

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
		if (!IsValid(CurrentGrappleSocket))
		{
			return false;
		}
	}

	//Attach the pawn to the Grapple Socket
	CurrentGrappleSocket->AttachToGrappleSocket(InPawn);

	//Nothing went wrong, and it is safe to continue with animations or set other variables, etc.
	return true;
}

void UGrapplerComponent::DetachFromGrappleSocket(AMoverPawn* InPawn)
{
	if (IsValid(CurrentGrappleSocket) && IsValid(InPawn))
	{
		//If pawn is attached to a Grapple Socket, deattach from it
		if (InPawn->CharacterMoverComponent->IsAirborne())
		{
			//Deattach the pawn from the Grapple Socket with force
			CurrentGrappleSocket->DetachFromGrappleSocket(InPawn, true);
		}
		else
		{
			//Deattach the pawn from the Grapple Socket without force
			CurrentGrappleSocket->DetachFromGrappleSocket(InPawn, false);
		}
	}
}

AGrappleSocket* UGrapplerComponent::FindClosestGrappleSocket(const AMoverPawn* InPawn) const
{
	//Pawn must be valid to proceed
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:FindClosestGrappleSocket - InPawn is not valid"), *StaticClass()->GetName());
		return nullptr;
	}

	//Controller must be valid to proceed
	AController* Controller = InPawn->GetController();
	if (!IsValid(Controller))
	{
		return nullptr;
	}

	// Get camera info
	FVector CameraLocation;
	FRotator CameraRotation;
	Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector CameraForward = CameraRotation.Vector();

	//Initialize function-based variables
	AGrappleSocket* ClosestGrappleSocket = nullptr;
	float ClosestGrappleSocketDistance = FLT_MAX;

	//Gets all the Grapple Sockets in the world
	TArray<AActor*> FoundGrapples;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrappleSocket::StaticClass(), FoundGrapples);

	//Iterate through all the found Grapple Sockets to find the closest one and if pawn is in range to grapple
	for (AActor* GrappleSocketActor : FoundGrapples)
	{
		//Get Grapple Socket
		AGrappleSocket* GrappleSocket = Cast<AGrappleSocket>(GrappleSocketActor);
		if (!IsValid(GrappleSocket))
		{
			continue;
		}

		const FVector SocketLocation = GrappleSocket->GetActorLocation();
		const FVector ToSocket = (SocketLocation - CameraLocation).GetSafeNormal();

		const float Dot = FVector::DotProduct(CameraForward, ToSocket);
		const float MinDotThreshold = 0.5f; 

		//Calculate if Grapple Socket is within 60 degree view
		//(if player camera is actually looking at Grapple Socket)
		if (Dot < MinDotThreshold)
		{
			continue;
		}

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(InPawn);

		//Check if camera can see grapple socket without getting blocked by anything
		bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, SocketLocation, ECC_Visibility, Params );
		if (bHit && Hit.GetActor() != GrappleSocket)
		{
			continue; 
		}

		//Check if pawn can see grapple socket without getting blocked by anything
		bool bHitWall = GetWorld()->LineTraceSingleByChannel(Hit, InPawn->GetActorLocation(), SocketLocation,ECC_Visibility, Params);
		if (bHitWall && Hit.GetActor() != GrappleSocket)
		{
			continue; 
		}

		//Compare distance to get the closest Grapple Point to pawn
		const float DistanceToPawn = GrappleSocket->GetDistanceFromAPawn(InPawn);
		if (DistanceToPawn < ClosestGrappleSocketDistance && GrappleSocket->IsInRangeToGrapple(InPawn))
		{
			ClosestGrappleSocket = GrappleSocket;
			ClosestGrappleSocketDistance = DistanceToPawn;
		}
	}

	//If after all the searching nothing was found, log it
	if (!IsValid(ClosestGrappleSocket))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s:FindClosestGrappleSocket - No valid grapple socket found"), *StaticClass()->GetName());
		return nullptr;
	}

	//Return the found Grapple Socket
	return ClosestGrappleSocket;
}
