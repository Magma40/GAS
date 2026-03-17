// Fill out your copyright notice in the Description page of Project Settings.


#include "GrapplerComponent.h"
#include "GrappleSocket.h"
#include "CableComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "MoverPawn.h"

// Sets default values for this component's properties
UGrapplerComponent::UGrapplerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	if (AMoverPawn* OwnerPawnSearch = Cast<AMoverPawn>(GetOwner()))
	{
		OwnerPawn = OwnerPawnSearch;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:UGrapplerComponent - GetOwner not found"), *StaticClass()->GetName());
		return;
	}
	
	if (IsValid(OwnerPawn) && IsValid(OwnerPawn->SkeletalMeshComponent))
	{
		OwnerPawn->GrapplerComponent = this;
		GrappleRope = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
		GrappleRope->SetupAttachment(OwnerPawn->SkeletalMeshComponent);
		GrappleRope->AttachToComponent(OwnerPawn->SkeletalMeshComponent , FAttachmentTransformRules::SnapToTargetNotIncludingScale, "hand_rSocket");
		GrappleRope->SetAttachEndTo(OwnerPawn, OwnerPawn->SkeletalMeshComponent->GetFName(), "hand_rSocket");
	}
	// ...
}


// Called when the game starts
void UGrapplerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AMoverPawn* OwnerPawnSearch = Cast<AMoverPawn>(GetOwner()))
	{
		OwnerPawn = OwnerPawnSearch;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - GetOwner not found"), *StaticClass()->GetName());
		return;
	}
	// ...
	
}


// Called every frame
void UGrapplerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UGrapplerComponent::TryToAttachToGrappleSocket()
{
	if (!IsValid(CurrentGrappleSocket))
	{
		CurrentGrappleSocket = FindClosestGrappleSocket();
		if (!IsValid(CurrentGrappleSocket)) return false;
	}

	bDoingGrapplingAction = true;
	ConstructGrappleRope();
	CurrentGrappleSocket->AttachToGrappleSocket(OwnerPawn);
	return true;
}

void UGrapplerComponent::DeattachFromGrappleSocket()
{
	if (IsValid(CurrentGrappleSocket) && IsValid(GrappleRope))
	{
		bDoingGrapplingAction = false;
		CurrentGrappleSocket->DetachToGrappleSocket(OwnerPawn);
		CurrentGrappleSocket = nullptr;
		GrappleRope->NumSegments = 0;
	}
}

AGrappleSocket* UGrapplerComponent::FindClosestGrappleSocket() const
{
	if (!IsValid(OwnerPawn)) return nullptr;

	AGrappleSocket* ClosestGrappleSocket = nullptr;
	float ClosestGrappleSocketDistance = FLT_MAX;
	
	TArray<AActor*> FoundGrapples;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrappleSocket::StaticClass(), FoundGrapples);
	
	for (AActor* GrappleSocketActor : FoundGrapples)
	{
		if (!IsValid(GrappleSocketActor)) continue;
		AGrappleSocket* GrappleSocket = Cast<AGrappleSocket>(GrappleSocketActor);
		if (!IsValid(GrappleSocket)) continue;

		const float DistanceToPawn = GrappleSocket->GetDistanceFromAPawn(OwnerPawn);
		if (DistanceToPawn < ClosestGrappleSocketDistance && GrappleSocket->IsInRangeToGrapple(OwnerPawn))
		{
			ClosestGrappleSocket = GrappleSocket;
			ClosestGrappleSocketDistance = DistanceToPawn;
		}
	}

	if (!IsValid(ClosestGrappleSocket))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:TryToAttachToGrappleSocket - ClosestGrappleSocket not valid or not any Grapple Sockets in world"), *StaticClass()->GetName());
		return nullptr;
	}
	
	return ClosestGrappleSocket;
}

void UGrapplerComponent::ConstructGrappleRope() const
{
	if (IsValid(GrappleRope) && IsValid(OwnerPawn) && IsValid(CurrentGrappleSocket))
	{
		USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(OwnerPawn->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		if (IsValid(SkeletalMeshComponent))
		{
			GrappleRope->AttachToComponent(SkeletalMeshComponent , FAttachmentTransformRules::SnapToTargetNotIncludingScale, "hand_rSocket");
			GrappleRope->CableWidth = 5.0f;
			GrappleRope->NumSegments = 20;

			const float CableLength = FVector::Dist(CurrentGrappleSocket->GetActorLocation(), OwnerPawn->GetActorLocation());
			GrappleRope->CableLength = CableLength;

			GrappleRope->bEnableCollision = true;

			GrappleRope->EndLocation = CurrentGrappleSocket->GetActorLocation();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:ConstructGrappleRope - GrappleRope, OwnerPawn or CurrentGrappleSocket not valid"), *StaticClass()->GetName());
		return;
	}
	
}

