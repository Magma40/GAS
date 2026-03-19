// Fill out your copyright notice in the Description page of Project Settings.


#include "GrapplerComponent.h"
#include "GrappleSocket.h"
#include "CableComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "MoverPawn.h"
#include "Components/CapsuleComponent.h"
//#include "CharacterMoverComponent.h"

// Sets default values for this component's properties
UGrapplerComponent::UGrapplerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	 PrimaryComponentTick.bCanEverTick = true;

	//Gets the Owner Pawn
	//this also has a chance to be null, but OwnerPawn will still be valid next if check
	AMoverPawn* OwnerPawnSearch =  Cast<AMoverPawn>(GetOwner());
	if (!IsValid(OwnerPawnSearch))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:UGrapplerComponent - GetOwner() not found"), *StaticClass()->GetName());
		return;
	}

	//Checks if everything is valid before constructing a Cable Component
	//this construction can be quite buggy, in most cases you can continue if you hit a "crash breakpoint"
	if (IsValid(OwnerPawnSearch) && !IsValid(GrappleRope))
	{
		//Safety set reference, this might not be necessary
		OwnerPawnSearch->GrapplerComponent = this;

		//Constructs the Cable Component
		GrappleRope = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent_Rope"));
		if (IsValid(GrappleRope) && IsValid(OwnerPawn) && IsValid(OwnerPawn->SkeletalMeshComponent))
		{
			//Attaches the Grapple Component to the Owners Skeletal Mesh Components sockets, in this case the right hand
			const FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, false);
			GrappleRope->AttachToComponent(OwnerPawn->SkeletalMeshComponent, AttachmentRules, "hand_rSocket");
			GrappleRope->SetAttachEndTo(OwnerPawn, OwnerPawn->SkeletalMeshComponent->GetFName(), "hand_rSocket");
		}
		else
		{
			//Debug logs the invalid components which prevented construction
			UE_LOG(LogTemp, Error,
				TEXT("%s: UGrapplerComponent - Invalid components:\n")
				TEXT("  GrappleRope: %s\n")
				TEXT("  OwnerPawnSearch: %s\n")
				TEXT("  SkeletalMeshComponent: %s"),
			    
				*StaticClass()->GetName(),
				GrappleRope ? *GrappleRope->GetName() : TEXT("nullptr"),
				OwnerPawnSearch ? *OwnerPawnSearch->GetName() : TEXT("nullptr"),
				(OwnerPawn && OwnerPawn->SkeletalMeshComponent) ? *OwnerPawn->SkeletalMeshComponent->GetName() : TEXT("nullptr")
			);
			return;
		}
	}
}

// Called when the game starts
void UGrapplerComponent::BeginPlay()
{
	Super::BeginPlay();

	//Timer Which calls Init function after a set time
	//Some components have a chance to not be properly setup or initialized at construction or BeginPlay, adding a delay can prevent this
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UGrapplerComponent::Init, 1.0f, false, 2.0f);
}

void UGrapplerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsValid(CurrentGrappleSocket))
	{
		if (!CurrentGrappleSocket->IsInRangeToGrapple(OwnerPawn) || CurrentGrappleSocket->GetDistanceFromAPawn(OwnerPawn)  > MaxGrapplerRopeLength)
		{
			DeattachFromGrappleSocket();
		}
	}
}

//NOTE: I Attach the Grappler Rope to the Skeletal Mesh Component through blueprint (Mover.SandboxCharacter_Mover - BeginPlay Event)
//NOTE: For some reason it works in blueprint but not in C++, its unlucky ):
void UGrapplerComponent::Init()
{
	//Gets the Owner Pawn reference if it is not already initialized
	APawn* PlayerSearch = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (IsValid(PlayerSearch)) OwnerPawn = Cast<AMoverPawn>(PlayerSearch);
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:Init - OwnerPawn not found"), *StaticClass()->GetName());
		return;
	}

	//Gets the Grapple Rope reference if it is not already initialized 
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

	CachedDefaultRopeLength = GrappleRope->CableLength;
}

bool UGrapplerComponent::TryToAttachToGrappleSocket()
{
	//Checks if pawn is able to grapple onto a Grapple Socket
	if (!IsValid(CurrentGrappleSocket))
	{
		CurrentGrappleSocket = FindClosestGrappleSocket();
		if (!IsValid(CurrentGrappleSocket)) return false;
	}

	//Enable Grapple Action
	bDoingGrapplingAction = true;

	//Construct a rope based on the found Grapple Socket
	//ConstructGrappleRope();
	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, false);
	OwnerPawn->CapsuleComponent->AttachToComponent(CurrentGrappleSocket->GrappleEdgeComponent, AttachmentRules);
	OwnerPawn->CapsuleComponent->SetRelativeLocation(FVector::ZeroVector);
	//UCharacterMoverComponent* CharacterMovementComponent = OwnerPawn->GetComponentByClass<UCharacterMoverComponent>();
	//if (IsValid(CharacterMovementComponent)) CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);

	//Attach the pawn to the Grapple Socket
	CurrentGrappleSocket->AttachToGrappleSocket(OwnerPawn);

	//Nothing went wrong, and it is safe to continue with animations or set other variables, etc.
	return true;
}

void UGrapplerComponent::DeattachFromGrappleSocket()
{
	//If pawn is attached to a Grapple Socket, deattach from it
	if (IsValid(CurrentGrappleSocket) && IsValid(GrappleRope))
	{
		//Disable Grapple Action
		bDoingGrapplingAction = false;

		//Deattach the pawn from the Grapple Socket
		CurrentGrappleSocket->DetachFromGrappleSocket(OwnerPawn);

		//Dereference the Grapple Socket pawn was attached to
		CurrentGrappleSocket = nullptr;

		//Reattach the Grapple Rope to the player
		GrappleRope->SetAttachEndTo(OwnerPawn, OwnerPawn->SkeletalMeshComponent->GetFName(), "hand_rSocket");

		//Set the Grapple Rope's length to a reasonable value
		GrappleRope->CableLength = CachedDefaultRopeLength;
	}
}

AGrappleSocket* UGrapplerComponent::FindClosestGrappleSocket() const
{
	if (!IsValid(OwnerPawn)) return nullptr;

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
		const float DistanceToPawn = GrappleSocket->GetDistanceFromAPawn(OwnerPawn);
		if (DistanceToPawn < ClosestGrappleSocketDistance && GrappleSocket->IsInRangeToGrapple(OwnerPawn))
		{
			ClosestGrappleSocket = GrappleSocket;
			ClosestGrappleSocketDistance = DistanceToPawn;
		}
	}

	//If there was no Grapple Sockets in the world or pawn was not in range to grapple onto a Grapple Socket, return nothing
	if (!IsValid(ClosestGrappleSocket))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:TryToAttachToGrappleSocket - ClosestGrappleSocket not valid or not any Grapple Sockets in world"), *StaticClass()->GetName());
		return nullptr;
	}

	//Return the closest Grapple Socket to the pawn
	return ClosestGrappleSocket;
}

void UGrapplerComponent::ConstructGrappleRope() const
{
	if (IsValid(GrappleRope) && IsValid(OwnerPawn) && IsValid(CurrentGrappleSocket) && IsValid(OwnerPawn->SkeletalMeshComponent))
	{
			//Construct the rope length from the Grapple Rope to the Grapple Socket
			//This is done is relative transform because I assume CableComponents->CableLength is valued in relative from CableComponents location
			const FTransform GrappleRopesWorldTransform = GrappleRope->GetComponentTransform();
			const FVector RelativeLocationToGrappleSocket = GrappleRopesWorldTransform.InverseTransformPosition(CurrentGrappleSocket->Root->GetComponentLocation());
			float CableLength = RelativeLocationToGrappleSocket.Size() - GrapplerRopeExtraShaveOffFromOrAddToLength;
			GrappleRope->CableLength = CableLength;
		
			//Attach the Grapple Rope's end to the Grapple Socket
			//For some reason doing GetRootComponent() on Grapple Socket returns nullptr, safer to cache the root
			GrappleRope->SetAttachEndTo(CurrentGrappleSocket, CurrentGrappleSocket->Root->GetFName());
	}
	else
	{
		//Debug the components that return nullptr
		UE_LOG(LogTemp, Error,
			TEXT("%s: ConstructGrappleRope - Invalid components:\n")
			TEXT("  GrappleRope: %s\n")
			TEXT("  OwnerPawn: %s\n")
			TEXT("  CurrentGrappleSocket: %s\n")
			TEXT("  SkeletalMeshComponent: %s"),
    
			*StaticClass()->GetName(),
			GrappleRope ? *GrappleRope->GetName() : TEXT("nullptr"),
			OwnerPawn ? *OwnerPawn->GetName() : TEXT("nullptr"),
			CurrentGrappleSocket ? *CurrentGrappleSocket->GetName() : TEXT("nullptr"),
			(OwnerPawn && OwnerPawn->SkeletalMeshComponent) ? *OwnerPawn->SkeletalMeshComponent->GetName() : TEXT("nullptr")
		);
		return;
	}
}

