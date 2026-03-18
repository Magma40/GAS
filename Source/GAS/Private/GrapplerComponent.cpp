// Fill out your copyright notice in the Description page of Project Settings.


#include "GrapplerComponent.h"
#include "GrappleSocket.h"
#include "CableComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "MoverPawn.h"

// Sets default values for this component's properties
UGrapplerComponent::UGrapplerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	 PrimaryComponentTick.bCanEverTick = true;
	// SetIsReplicatedByDefault(true);
	
	if (AMoverPawn* OwnerPawnSearch = Cast<AMoverPawn>(GetOwner()))
	{
		OwnerPawn = OwnerPawnSearch;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:UGrapplerComponent - GetOwner not found"), *StaticClass()->GetName());
		return;
	}
	
	if (IsValid(OwnerPawn) && !IsValid(GrappleRope))
	{
		OwnerPawn->GrapplerComponent = this;
		GrappleRope = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent_Rope"));
		if (IsValid(GrappleRope) && IsValid(OwnerPawn) && IsValid(OwnerPawn->SkeletalMeshComponent))
		{
			// GrappleRope->bEnableCollision = false;
			// GrappleRope->SetCollisionProfileName(TEXT("NoCollision"));
			// GrappleRope->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, false);
			GrappleRope->AttachToComponent(OwnerPawn->SkeletalMeshComponent, AttachmentRules, "hand_rSocket");
			GrappleRope->SetAttachEndTo(OwnerPawn, OwnerPawn->SkeletalMeshComponent->GetFName(), "hand_rSocket");
		}
	}
}

// Called when the game starts
void UGrapplerComponent::BeginPlay()
{
	Super::BeginPlay();
	FTimerHandle TimerHandle;

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UGrapplerComponent::Init, 1.0f, false, 2.0f);
}

void UGrapplerComponent::Init()
{
	if (!IsValid(OwnerPawn))
	{
		AMoverPawn* OwnerPawnSearch = Cast<AMoverPawn>(GetOwner());
		if (IsValid(OwnerPawnSearch))  OwnerPawn = OwnerPawnSearch;
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - GetOwner not found"), *StaticClass()->GetName());
			return;
		}
	}
	
	if (!IsValid(GrappleRope) && IsValid(OwnerPawn->SkeletalMeshComponent))
	{
		UCableComponent* GrappleRopeSearch = Cast<UCableComponent>(OwnerPawn->GetComponentByClass<UCableComponent>());
		if (IsValid(GrappleRopeSearch)) GrappleRope = GrappleRopeSearch;
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - GrappleRope not found"), *StaticClass()->GetName());
			return;
		}
	}

	// if (IsValid(GrappleRope) && IsValid(OwnerPawn->SkeletalMeshComponent))
	// {
	// 		//GrappleRope->bEnableCollision = false;
	// 		// GrappleRope->SetCollisionProfileName(TEXT("NoCollision"));
	// 		// GrappleRope->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 	
	// 		FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	// 		GrappleRope->AttachToComponent(OwnerPawn->SkeletalMeshComponent, AttachmentRules, TEXT("hand_rSocket"));
	// 		GrappleRope->SetAttachEndTo(OwnerPawn, OwnerPawn->SkeletalMeshComponent->GetFName(), "hand_rSocket");
	// 	
	// 		GrappleRope->SetRelativeLocation(FVector::ZeroVector);
	// 		GrappleRope->EndLocation = FVector::ZeroVector;
	// 		
	// 		UMaterialInterface* LoadedMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT( "/Script/Engine.Material'/Engine/MapTemplates/Sky/M_BlackBackground.M_BlackBackground'")));
	// 		if (LoadedMaterial)
	// 		{
	// 			GrappleRope->SetMaterial(0, LoadedMaterial);
	// 		}
	// }
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

		GrappleRope->SetAttachEndTo(OwnerPawn, OwnerPawn->SkeletalMeshComponent->GetFName(), "hand_rSocket");
		GrappleRope->CableLength = 100.0f;

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
	if (IsValid(GrappleRope) && IsValid(OwnerPawn) && IsValid(CurrentGrappleSocket) && IsValid(OwnerPawn->SkeletalMeshComponent))
	{
			GrappleRope->CableWidth = 5.0f;
			//GrappleRope->NumSegments = 20;

			const FTransform Xform = GrappleRope->GetComponentTransform();
			FVector RelativeLocationToGrappleSocket = Xform.InverseTransformPosition(CurrentGrappleSocket->Root->GetComponentLocation());
			 //const float CableLength = FVector::Dist(RelativeLocationToGrappleSocket, OwnerPawn->GetActorLocation());
			 GrappleRope->CableLength = RelativeLocationToGrappleSocket.Size();

			//GrappleRope->bEnableCollision = true;

			//GrappleRope->EndLocation = CurrentGrappleSocket->GetActorLocation();
			GrappleRope->SetAttachEndTo(CurrentGrappleSocket, CurrentGrappleSocket->Root->GetFName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:ConstructGrappleRope - GrappleRope, OwnerPawn or CurrentGrappleSocket not valid"), *StaticClass()->GetName());
		return;
	}
	
}

