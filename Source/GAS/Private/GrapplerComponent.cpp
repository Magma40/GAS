// Fill out your copyright notice in the Description page of Project Settings.
#include "GrapplerComponent.h"
#include "GrappleSocket.h"
#include "CableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MoverPawn.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DefaultMovementSet/LayeredMoves/LaunchMove.h"

UE_DEFINE_GAMEPLAY_TAG(Mover_IsGrappling, "Mover.IsGrappling");

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
	
	//Destroy Components for reconstruction
	UCableComponent* CableComponent = OwnerPawnSearch->GetComponentByClass<UCableComponent>();
	if (IsValid(CableComponent))
	{
		CableComponent->DestroyComponent();	
	}
	
	//Checks if everything is valid before constructing a Cable Component
	//this construction can be quite buggy, in most cases you can continue if you hit a "crash breakpoint"
	if (IsValid(OwnerPawnSearch) && !IsValid(GrappleRope))
	{
		//Safety set reference, this might not be necessary
		OwnerPawnSearch->GrapplerComponent = this;
	
		//Constructs the Cable Component
		GrappleRope = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent_Rope"));
		if (IsValid(GrappleRope) && IsValid(OwnerPawnSearch) && IsValid(OwnerPawnSearch->SkeletalMeshComponent))
		{
			//Attaches the Grapple Component to the Owners Skeletal Mesh Components sockets, in this case the right hand
			const FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, false);
			GrappleRope->AttachToComponent(OwnerPawnSearch->SkeletalMeshComponent, AttachmentRules, "hand_rSocket");
			GrappleRope->SetAttachEndTo(OwnerPawnSearch, OwnerPawnSearch->SkeletalMeshComponent->GetFName(), "hand_rSocket");
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
				(OwnerPawnSearch && OwnerPawnSearch->SkeletalMeshComponent) ? *OwnerPawnSearch->SkeletalMeshComponent->GetName() : TEXT("nullptr")
			);
			return;
		}
	}
	//Debug log that the object has been constructed
	UE_LOG(LogTemp, Warning, TEXT("%s:UGrapplerComponent - %s Constructed:"), *StaticClass()->GetName(), *this->GetName());
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
			DetachFromGrappleSocket();
		}
	}

	if (IsValid(OwnerPawn) && IsValid(OwnerPawn->CharacterMoverComponent))
	{
		if (OwnerPawn->CharacterMoverComponent->IsFalling() && bDoingGrapplingAction)
		{
			AttachPawnToGrappleSocket();
			OwnerPawn->SetActorRelativeLocation(FVector::ZeroVector);
			EnableGrappling();
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
	ConstructGrappleRope();

	//Attach the pawn to the Grapple Socket
	CurrentGrappleSocket->AttachToGrappleSocket(OwnerPawn);

	//Nothing went wrong, and it is safe to continue with animations or set other variables, etc.
	return true;
}

void UGrapplerComponent::DetachFromGrappleSocket()
{
	//If pawn is attached to a Grapple Socket, deattach from it
	if (IsValid(CurrentGrappleSocket) && IsValid(GrappleRope))
	{
		//Disable Grapple Action
		bDoingGrapplingAction = false;

		DetachPawnFromGrappleSocket();
		if (IsValid(CurrentGrappleSocket) && IsValid(CurrentGrappleSocket->GrappleRope))
		{
			CurrentGrappleSocket->GrappleRope->SetVisibility(false);
		}

		//Deattach the pawn from the Grapple Socket
		CurrentGrappleSocket->DetachFromGrappleSocket(OwnerPawn);

		//Dereference the Grapple Socket pawn was attached to
		CurrentGrappleSocket = nullptr;
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

void UGrapplerComponent::AttachPawnToGrappleSocket() const
{
	if (IsValid(OwnerPawn) && IsValid(OwnerPawn->CapsuleComponent) && IsValid(CurrentGrappleSocket) && IsValid(CurrentGrappleSocket->GrappleEdgeComponent))
	{
		const FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, false);
		OwnerPawn->CapsuleComponent->AttachToComponent(CurrentGrappleSocket->GrappleEdgeComponent, AttachmentRules);
		OwnerPawn->CapsuleComponent->SetRelativeLocation(FVector::ZeroVector);

		OwnerPawn->CharacterMoverComponent->AddGameplayTag(Mover_IsGrappling);
	}
	else
	{
		//Debug the components that return nullptr
		UE_LOG(LogTemp, Error,
			TEXT("%s: AttachPawnToGrappleSocket - Invalid components:\n")
			TEXT("  OwnerPawn: %s\n")
			TEXT("  OwnerPawn->CapsuleComponent: %s\n")
			TEXT("  CurrentGrappleSocket: %s\n")
			TEXT("  CurrentGrappleSocket->GrappleEdgeComponent: %s\n"),
    
			*StaticClass()->GetName(),
			OwnerPawn ? *OwnerPawn->GetName() : TEXT("nullptr"),
			(OwnerPawn && OwnerPawn->CapsuleComponent) ? *OwnerPawn->CapsuleComponent->GetName() : TEXT("nullptr"),
			CurrentGrappleSocket ? *CurrentGrappleSocket->GetName() : TEXT("nullptr"),
			(CurrentGrappleSocket && CurrentGrappleSocket->GrappleEdgeComponent) ? *CurrentGrappleSocket->GrappleEdgeComponent->GetName() : TEXT("nullptr")
		);
		return;
	}
}

void UGrapplerComponent::DetachPawnFromGrappleSocket() const
{
	if (IsValid(OwnerPawn) && IsValid(CurrentGrappleSocket))
	{
		CurrentGrappleSocket->GrappleRope->bAttachEnd = false;
		const FDetachmentTransformRules AttachmentRules(FDetachmentTransformRules::KeepWorldTransform);
		OwnerPawn ->DetachFromActor(AttachmentRules); //  ->DetachFromComponent(AttachmentRules);

		OwnerPawn->CharacterMoverComponent->RemoveGameplayTag(Mover_IsGrappling);

		TSharedPtr<FLayeredMove_Launch> JumpLayeredMove = MakeShared<FLayeredMove_Launch>();
		JumpLayeredMove->ForceMovementMode = "Falling";
		JumpLayeredMove->LaunchVelocity = FVector(0, 0, 1000.0f);
		JumpLayeredMove->DurationMs = 0.0f;

		FMoverTickStartData MoverTickStartData = FMoverTickStartData();
		MoverTickStartData.InputCmd = OwnerPawn->CharacterMoverComponent->GetLastInputCmd();
		MoverTickStartData.SyncState = OwnerPawn->CharacterMoverComponent->GetSyncState();
		MoverTickStartData.AuxState = OwnerPawn->CharacterMoverComponent->GetLastAuxStateContext();
				
		FProposedMove ProposedMove = FProposedMove();
		ProposedMove.PreferredMode = "Falling";
		ProposedMove.DirectionIntent = OwnerPawn->CharacterMoverComponent->GetVelocity().GetSafeNormal();

		UPrimitiveComponent* Base = OwnerPawn->CharacterMoverComponent->GetMovementBase();
		if (IsValid(Base) && OwnerPawn->CharacterMoverComponent->IsOnGround())
		{
			ProposedMove.LinearVelocity = Base->GetPhysicsLinearVelocity(NAME_None);
			ProposedMove.AngularVelocityDegrees = Base->GetPhysicsAngularVelocityInDegrees(NAME_None);
		}
		else
		{
			ProposedMove.LinearVelocity = OwnerPawn->CharacterMoverComponent->GetVelocity();
			ProposedMove.AngularVelocityDegrees = FVector::ZeroVector;
		}
		ProposedMove.bHasDirIntent = true;
		ProposedMove.MixMode = EMoveMixMode::AdditiveVelocity;
				
		JumpLayeredMove->GenerateMove(
			MoverTickStartData,
			OwnerPawn->CharacterMoverComponent->GetLastTimeStep(),
			OwnerPawn->CharacterMoverComponent,
			const_cast<UMoverBlackboard*>(OwnerPawn->CharacterMoverComponent->GetSimBlackboard()),
			ProposedMove
			);
				
		OwnerPawn->CharacterMoverComponent->QueueLayeredMove(JumpLayeredMove);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:DetachPawnToGrappleSocket - OwnerPawn or Current GrappleSocket not valid"), *StaticClass()->GetName());
		return;
	}
}

void UGrapplerComponent::ConstructGrappleRope()
{
	if (IsValid(GrappleRope) && IsValid(OwnerPawn) && IsValid(CurrentGrappleSocket) && IsValid(OwnerPawn->SkeletalMeshComponent))
	{
			//Construct the rope length from the Grapple Rope to the Grapple Socket
			//This is done is relative transform because I assume CableComponents->CableLength is valued in relative from CableComponents location
			if (IsValid(CurrentGrappleSocket) && IsValid(CurrentGrappleSocket->GrappleRope))
			{
				const FTransform GrappleSocketWorldTransform = CurrentGrappleSocket->GetActorTransform();
				FVector SocketLocation = OwnerPawn->SkeletalMeshComponent->GetSocketLocation("hand_rSocket");
				const FVector HandSocketRelativeLocationToGrappleSocket = GrappleSocketWorldTransform.InverseTransformPosition(SocketLocation);
				const float CableLength = FMath::Abs( HandSocketRelativeLocationToGrappleSocket.Size2D() - GrapplerRopeExtraShaveOffFromOrAddToLength);
				
				CurrentGrappleSocket->GrappleRope->CableLength = CableLength;
				CurrentGrappleSocket->GrappleRope->EndLocation = HandSocketRelativeLocationToGrappleSocket;
				
				CurrentGrappleSocket->GrappleRope->bAttachEnd = true;
				CurrentGrappleSocket->GrappleRope->SetAttachEndTo(OwnerPawn, OwnerPawn->SkeletalMeshComponent->GetFName(), "hand_rSocket");
				
				CurrentGrappleSocket->GrappleRope->SetVisibility(true);
			}
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

void UGrapplerComponent::EnableGrappling() const
{
	if (IsValid(CurrentGrappleSocket) && IsValid(CurrentGrappleSocket->GrappleRope))
	{
		CurrentGrappleSocket->GrappleRope->bAttachEnd = false;
	}
}

