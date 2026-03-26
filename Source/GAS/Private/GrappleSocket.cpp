// Fill out your copyright notice in the Description page of Project Settings.
#include "GrappleSocket.h"
#include "GrapplingSocketWidgetComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "MoverPawn.h"
#include "CableComponent.h"
#include "GrapplerComponent.h"
#include "Components/CapsuleComponent.h"

#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DefaultMovementSet/LayeredMoves/LaunchMove.h"

// Sets default values
AGrappleSocket::AGrappleSocket()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Destroy Components for reconstruction
	//Destroy Root Component
	USceneComponent* RootComponentFound = GetComponentByClass<USceneComponent>();
	if (IsValid(RootComponentFound))
	{
		RootComponentFound->DestroyComponent();	
	}

	//Destroy Static Mesh Component
	UStaticMeshComponent* StaticMeshComponentFound = GetComponentByClass<UStaticMeshComponent>();
	if (IsValid(StaticMeshComponentFound))
	{
		StaticMeshComponentFound->DestroyComponent();	
	}

	//Destroy Grappler Socket Widget Component
	UGrapplingSocketWidgetComponent* GrapplingSocketWidgetComponentFound = GetComponentByClass<UGrapplingSocketWidgetComponent>();
	if (IsValid(GrapplingSocketWidgetComponentFound))
	{
		GrapplingSocketWidgetComponentFound->DestroyComponent();	
	}
	
	//Destroy Cable Component
	UCableComponent* CableComponentFound = GetComponentByClass<UCableComponent>();
	if (IsValid(CableComponentFound))
	{
		CableComponentFound->DestroyComponent();	
	}

	//Destroy Capsule Component
	UCapsuleComponent* CapsuleComponentFound = GetComponentByClass<UCapsuleComponent>();
	if (IsValid(CapsuleComponentFound))
	{
		CapsuleComponentFound->DestroyComponent();	
	}

	//Construct Root Component
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	if (IsValid(Root))
	{
		SetRootComponent(Root);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: AGrappleSocket - Root is not valid"),*StaticClass()->GetName());
		return;
	}
	
	//Construct A StaticMesh
	StaticMeshComponent= CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	StaticMeshComponent->SetupAttachment(Root);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetHiddenInGame(false);

	if (!IsValid(StaticMeshComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: AGrappleSocket - StaticMeshComponent is not valid"),*StaticClass()->GetName());
		return;
	}
	//if (IsValid(StaticMeshRef)) StaticMeshComponent->SetStaticMesh(StaticMeshRef);
	
	
	//Try to find Unreal's default Sphere shape inside engine folders
	 static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	
	 //If found Unreal's default Sphere shape, apply it to the constructed static mesh 
	 if (SphereMesh.Succeeded())
	 {
	 	StaticMeshComponent->SetStaticMesh(SphereMesh.Object);
	 }
	 else
	 {
	 	UE_LOG(LogTemp, Error, TEXT("%s: AGrappleSocket - SphereMesh is not valid"),*StaticClass()->GetName());
	 	//return;
	 }
	
	GrapplingSocketWidgetComponent  = CreateDefaultSubobject<UGrapplingSocketWidgetComponent>(TEXT("GrapplingSocketWidgetComponent"));
	if (IsValid(GrapplingSocketWidgetComponent))
	{
		GrapplingSocketWidgetComponent->SetupAttachment(Root);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: AGrappleSocket - GrapplingSocketWidgetComponent is not valid"),*StaticClass()->GetName());
		return;
	}
	

	GrappleRope = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
	if (IsValid(GrappleRope) && IsValid(Root))
	{
		const FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, false);
		GrappleRope->AttachToComponent(Root, AttachmentRules);
	
		GrappleEdgeComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("GrappleCapsuleComponent"));
		GrappleEdgeComponent->AttachToComponent(GrappleRope , AttachmentRules, "CableEnd");
		//GrappleRope->SetAttachEndTo(this, GrappleEdgeComponent->GetFName());
	}
	else
	{
		//Debug logs the invalid components which prevented construction
		UE_LOG(LogTemp, Error,
			TEXT("%s: AGrappleSocket - Invalid components:\n")
			TEXT("  GrappleRope: %s\n")
			TEXT("  Root: %s\n"),
			    
			*StaticClass()->GetName(),
			GrappleRope ? *GrappleRope->GetName() : TEXT("nullptr"),
			Root ? *Root->GetName() : TEXT("nullptr")
		);
		return;
	}
	
	//Debug log that the object has been constructed
	UE_LOG(LogTemp, Warning, TEXT("%s:AGrappleSocket - %s Constructed:"), *StaticClass()->GetName(), *this->GetName());
}

// Called when the game starts or when spawned
void AGrappleSocket::BeginPlay()
{
	Super::BeginPlay();

	//Try to get the PlayerPawn in the world
	AMoverPawn* PlayerSearch = Cast<AMoverPawn>(GetWorld()->GetFirstPlayerController()->GetPawn()) ;
	if (!IsValid(PlayerSearch))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - PlayerSearch not found"), *StaticClass()->GetName());
		return;
	}

	//Try to get the Camera Component in pawn
	Cached_Camera = Cast<UCameraComponent>(PlayerSearch->GetComponentByClass(UCameraComponent::StaticClass()));
	if (!IsValid(Cached_Camera))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - Cached_Camera not found"), *StaticClass()->GetName());
		return;
	}

	//Try to get the Pawn's Player Controller
	const APlayerController* PC = Cast<APlayerController>(PlayerSearch->GetController());
	if (!IsValid(PC))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - Player Controller not found"), *StaticClass()->GetName());
		return;
	}
	
	//Try to get the Player Controller's Enhanced Input Component
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!IsValid(Input))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - Input not found"), *StaticClass()->GetName());
		return;
	}

	//Bind so every time the player moves, it should update the Widget and Grapple Rope
	Input->BindAction(PlayerSearch->GetMoveAction(), ETriggerEvent::Triggered, this, &AGrappleSocket::UpdateWidget);
	Input->BindAction(PlayerSearch->GetMoveAction(), ETriggerEvent::Triggered, this, &AGrappleSocket::UpdateGrappleRope);

	//Apply the visibility if they are not correct
	GrapplingSocketWidgetComponent->SetWidgetImageVisibility(bEnableImage);
	GrapplingSocketWidgetComponent->SetWidgetTextVisibility(bEnableText);
	GrappleRope->SetVisibility(false);
}

void AGrappleSocket::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!IsValid(GrapplingSocketWidgetComponent)) return;
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AGrappleSocket, bEnableImage))
		{
		/* Because you are inside the class, you should see the value already changed */
		if (bEnableImage) GrapplingSocketWidgetComponent->SetWidgetImageVisibility(true);
		else  GrapplingSocketWidgetComponent->SetWidgetImageVisibility(false);
		
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AGrappleSocket, bEnableText))
	{
		if (bEnableText) GrapplingSocketWidgetComponent->SetWidgetTextVisibility(true);
		else GrapplingSocketWidgetComponent->SetWidgetTextVisibility(false);
	}
}

void AGrappleSocket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(!IsValid(Cached_PlayerPawn) || !IsValid(GrappleRope)) return;

	//If Pawn is not in range or is breaking the rope physics, detach it from the Grapple Socket
	if (!IsInRangeToGrapple(Cached_PlayerPawn) || GetDistanceFromAPawn(Cached_PlayerPawn)  > MaxGrapplerRopeLength)
	{
		DetachFromGrappleSocket(Cached_PlayerPawn, false);
		return;
	}
}

void AGrappleSocket::UpdateWidget()
{
	if(!IsValid(GrapplingSocketWidgetComponent) || !IsValid(Cached_Camera) || !IsValid(StaticMeshComponent) && !IsValid(GrappleRope)) return;

	AMoverPawn* PlayerSearch = Cast<AMoverPawn>(GetWorld()->GetFirstPlayerController()->GetPawn()); 
	if (!IsValid(PlayerSearch)) return;
	
	//If Pawn is in the range to grapple, display the widget
	if(IsInRangeToGrapple(PlayerSearch))
	{
		//Calculate the location of the edge of the Grapple Socket mesh 
		const FVector Direction = (PlayerSearch->GetActorLocation() -  GetActorLocation()).GetSafeNormal();
		const FBoxSphereBounds Bounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
		const float Radius = Bounds.SphereRadius;
		const FVector NewLocation = GetActorLocation() + Direction * Radius;

		//Apply new Widget Location so it traces around the Grapple Socket and is always visible and facing towards the Pawn
		GrapplingSocketWidgetComponent->SetWorldLocation(NewLocation);
		GrapplingSocketWidgetComponent->SetWorldRotation((Cached_Camera->GetComponentLocation() - GrapplingSocketWidgetComponent->GetComponentLocation()).Rotation());

		//When everything is set, make the Widget visible to Pawn
		GrapplingSocketWidgetComponent->SetWidgetVisibility(true);
	}
	else
	{
		//No need to show the Widget to Pawn if Pawn is not in range
		GrapplingSocketWidgetComponent->SetWidgetVisibility(false);
	}
}

void AGrappleSocket::UpdateGrappleRope()
{
	AMoverPawn* PlayerSearch = Cast<AMoverPawn>(GetWorld()->GetFirstPlayerController()->GetPawn());
	if (!IsValid(PlayerSearch) || !IsValid(PlayerSearch->CharacterMoverComponent)) return;
	
	if(IsInRangeToGrapple(PlayerSearch) &&
		!PlayerSearch->CharacterMoverComponent->IsFalling() &&
		!PlayerSearch->GrapplerComponent->GetDoingGrapplingAction())
	{
		//Construct a rope based on the pawn's location'
		Cached_PawnLocation  = PlayerSearch->GetActorLocation();
		ConstructGrappleRope(PlayerSearch);
	}
	else
	{
		Cached_PawnLocation  = PlayerSearch->GetActorLocation();
		EnableGrappling();
	}
}

bool AGrappleSocket::IsInRangeToGrapple(const AMoverPawn* InPawn) const
{
	const float Dist = GetDistanceFromAPawn(InPawn);
	return Dist <= MinDistanceToGrapple;
}

float AGrappleSocket::GetDistanceFromAPawn(const AMoverPawn* InPawn) const
{
	FVector ToInteractionLocation = GetActorLocation() - InPawn->GetActorLocation();
	ToInteractionLocation.Z = 0.0f; // ignore vertical difference
	return ToInteractionLocation.Size();
}

void AGrappleSocket::AttachToGrappleSocket(AMoverPawn* InPawn)
{
	if (IsValid(InPawn) && IsValid(InPawn->GrapplerComponent))
	{
		//Set the PlayerPawn
		Cached_PlayerPawn = InPawn;
		//Set that Pawn is currently doing Grappling Action
		InPawn->GrapplerComponent->SetDoingGrapplingAction(true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:AttachToGrappleSocket - InPawn not valid"), *StaticClass()->GetName());
		return;
	}

	//Disable the widgets for the pawn while they are grappling
	if (IsValid(GrapplingSocketWidgetComponent)) GrapplingSocketWidgetComponent->SetWidgetVisibility(false);
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:AttachToGrappleSocket - GrapplingSocketWidgetComponent not valid"), *StaticClass()->GetName());
		return;
	}
	
	if(IsValid(GEngine))
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Grappled onto %s"), *this->GetName()));
	}

	if (IsValid(GrappleRope))
	{
		//Once the Grapple Rope has been fully constructed, make it visible
		GrappleRope->SetVisibility(true);
	}
}

void AGrappleSocket::AttachPawnToGrappleSocket(AMoverPawn* InPawn) const
{
	if (IsValid(InPawn) && IsValid(InPawn->CapsuleComponent) && IsValid(GrappleEdgeComponent))
	{
		//Attaches the Pawn (CapsuleComponent = RootComponent), to the end part of the rope
		const FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, false);
		InPawn->CapsuleComponent->AttachToComponent(GrappleEdgeComponent, AttachmentRules);
		InPawn->CapsuleComponent->SetRelativeLocation(FVector::ZeroVector);

		//Add the GameplayTag IsGrappling to enable animations
		//This also changes the current MoverComponent movement mode (IsGrappling = Flying)
		InPawn->CharacterMoverComponent->AddGameplayTag(Mover_IsGrappling);
	}
	else
	{
		//Debug the components that return nullptr
		UE_LOG(LogTemp, Error,
			TEXT("%s: AttachToGrappleSocket - Invalid components:\n")
			TEXT("  InPawn: %s\n")
			TEXT("  InPawn->CapsuleComponent: %s\n")
			TEXT("  GrappleEdgeComponent: %s\n"),
    
			*StaticClass()->GetName(),
			InPawn ? *InPawn->GetName() : TEXT("nullptr"),
			(InPawn && InPawn->CapsuleComponent) ? *InPawn->CapsuleComponent->GetName() : TEXT("nullptr"),
			GrappleEdgeComponent ? *GrappleEdgeComponent->GetName() : TEXT("nullptr")
			);
		return;
	}
}

void AGrappleSocket::DetachFromGrappleSocket(AMoverPawn* InPawn, bool bApplyForce)
{
	Cached_PlayerPawn = nullptr;
	if(IsValid(InPawn) && IsValid(InPawn->GrapplerComponent))
	{
		//Set that Pawn is currently not doing Grappling Action anymore
		InPawn->GrapplerComponent->SetDoingGrapplingAction(false);

		//Deference the Current Grapple Socket inside Pawns Grapple Component
		InPawn->GrapplerComponent->SetCurrentGrappleSocket(nullptr);

		//Reset Cached Pawn Location for when Pawn attaches again
		Cached_PawnLocation = FVector::ZeroVector;

		//Reset Grappling Timer Handle for when Pawn attaches again
		EnableGrapplingTimerHandle.Invalidate();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:DetachFromGrappleSocket - InPawn not valid"), *StaticClass()->GetName());
		return;
	}

	//Enable the widgets for the pawn while they are not grappling anymore
	if (IsValid(GrapplingSocketWidgetComponent)) GrapplingSocketWidgetComponent->SetWidgetVisibility(true);
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:DetachFromGrappleSocket - GrapplingSocketWidgetComponent not valid"), *StaticClass()->GetName());
		return;
	}
	
	if(IsValid(GEngine))
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Grappled off from %s"), *this->GetName()));
	}

	if (IsValid(InPawn) && IsValid(InPawn->CharacterMoverComponent) && IsValid(GrappleEdgeComponent))
	{
		//Detaches the Pawn from the Grapple Socket
		GrappleRope->bAttachEnd = false;
		const FDetachmentTransformRules AttachmentRules(FDetachmentTransformRules::KeepWorldTransform);
		InPawn->DetachFromActor(AttachmentRules);

		//Removes the GameplayTag IsGrappling to disable animations
		//This also changes the current MoverComponent movement mode (IsGrappling = Flying)
		InPawn->CharacterMoverComponent->RemoveGameplayTag(Mover_IsGrappling);

		if (bApplyForce)
		{
			//Create a velocity based on how much energy the Pawn has generated
			const FVector  GrappleEdgeComponentLocation = GrappleEdgeComponent->GetComponentLocation();
			FVector GrappleEdgeComponentRelativeLocationToGrappleSocket = GetActorTransform().InverseTransformPosition(GrappleEdgeComponentLocation);

			//Makes sure Pawn always gets a bump when detaching from Grapple Socket
			if (GrappleEdgeComponentRelativeLocationToGrappleSocket.Z < 0 )
			{
				const float PositiveZValue = GrappleEdgeComponentRelativeLocationToGrappleSocket.Z * -1;
				GrappleEdgeComponentRelativeLocationToGrappleSocket.Z = PositiveZValue;
			}
			
			//Construct the JumpLayeredMove and all the necessary holders and variables
			//FLayeredMove's are forces which can be applied onto the Mover Component (adding a layer of force onto the movement)
			//This will cause it to not break the Mover Component movement and its animations
			//The LayeredMove also has a lifetime so its limited applied force
			TSharedPtr<FLayeredMove_Launch> JumpLayeredMove = ConstructGrappleMove(InPawn, GrappleEdgeComponentRelativeLocationToGrappleSocket);

			if (JumpLayeredMove)
			{
				//Queue the newly constructed JumpLayeredMove into the Movement
				InPawn->CharacterMoverComponent->QueueLayeredMove(JumpLayeredMove);
			}	
		}
	}
	else
	{
		//Debug the components that return nullptr
		UE_LOG(LogTemp, Error,
			TEXT("%s: DetachFromGrappleSocket - Invalid components:\n")
			TEXT("  InPawn: %s\n")
			TEXT("  InPawn->CharacterMoverComponent: %s\n"),
    
			*StaticClass()->GetName(),
			InPawn ? *InPawn->GetName() : TEXT("nullptr"),
			(InPawn && InPawn->CharacterMoverComponent) ? *InPawn->CharacterMoverComponent->GetName() : TEXT("nullptr")
		);
		return;
	}

	if (IsValid(GrappleRope))
	{
		//Disable the visibly of the rope
		GrappleRope->SetVisibility(false);
	}
}

void AGrappleSocket::ConstructGrappleRope(AMoverPawn* InPawn) const
{
	if (IsValid(InPawn) && IsValid(InPawn->SkeletalMeshComponent) && IsValid(GrappleRope))
	{
		//Construct the rope length from the Grapple Rope to the Grapple Socket
		//This is done is relative transform because I assume CableComponents->CableLength is valued in relative from CableComponents location
		FVector SocketLocation = InPawn->SkeletalMeshComponent->GetSocketLocation("hand_rSocket");
		const FVector HandSocketRelativeLocationToGrappleSocket = GetActorTransform().InverseTransformPosition(SocketLocation);

		//Set the Grapple Rope End Location to the relative position of Pawn's hand  to the Grapple Socket
		GrappleRope->EndLocation = HandSocketRelativeLocationToGrappleSocket;

		//Set the Grapple Rope Length to the distance between Grapple Socket and the Pawn's hand 
		const float CableLength = FMath::Abs( HandSocketRelativeLocationToGrappleSocket.Size2D() - GrapplerRopeExtraShaveOffFromOrAddToLength);
		GrappleRope->CableLength = CableLength;
		
		//Attach the Grapple Rope's end to the Pawn's hand	
		GrappleRope->bAttachEnd = true;
		GrappleRope->SetAttachEndTo(InPawn, InPawn->SkeletalMeshComponent->GetFName(), "hand_rSocket");
	}
	else
	{
		//Debug the components that return nullptr
		UE_LOG(LogTemp, Error,
			TEXT("%s: ConstructGrappleRope - Invalid components:\n")
			TEXT("  InPawn: %s\n")
			TEXT("  InPawn->SkeletalMeshComponent: %s\n")
			TEXT("  GrappleRope: %s\n"),
    
			*StaticClass()->GetName(),
			InPawn ? *InPawn->GetName() : TEXT("nullptr"),
			(InPawn && InPawn->SkeletalMeshComponent) ? *InPawn->SkeletalMeshComponent->GetName() : TEXT("nullptr"),
			GrappleRope ? *GrappleRope->GetName() : TEXT("nullptr")
		);
		return;
	}
}

TSharedPtr<FLayeredMove_Launch> AGrappleSocket::ConstructGrappleMove(const AMoverPawn* InPawn, const FVector& LaunchVelocity)
{
	if (IsValid(InPawn) && IsValid(InPawn->CharacterMoverComponent))
	{
		//Construct the base JumpLayered Move with the variable with variable which will simulate a jump
		TSharedPtr<FLayeredMove_Launch> JumpLayeredMove = MakeShared<FLayeredMove_Launch>();
		//Set the Movement the Mover Component should apply before applying new force
		JumpLayeredMove->ForceMovementMode = "Falling";
		//How much and in which direction the Mover Component should be go in
		JumpLayeredMove->LaunchVelocity = LaunchVelocity;
		//How many milliseconds should force be applied for (0.0f = once)
		JumpLayeredMove->DurationMs = 0.0f;

		//Construct holder based on the Mover Component's Input Data before it had begun moving during the tick
		FMoverTickStartData MoverTickStartData = FMoverTickStartData();
		MoverTickStartData.InputCmd = InPawn->CharacterMoverComponent->GetLastInputCmd();
		MoverTickStartData.SyncState = InPawn->CharacterMoverComponent->GetSyncState();
		//MoverTickStartData.AuxState = InPawn->CharacterMoverComponent->GetLastAuxStateContext();

		//Construct holder based on Mover Component's extra variables
		//This will be taken into consideration what the Mover Component should do during its newly applied Move
		FProposedMove ProposedMove = FProposedMove();
		//Set what the Movement for Mover Component should be after the applied Move is done
		ProposedMove.PreferredMode = "Falling";
		ProposedMove.DirectionIntent = InPawn->CharacterMoverComponent->GetVelocity().GetSafeNormal();

		//Try to get Mover Component's MovementBase
		//This will likely return null because MovementBase is set when Mover Component is standing on something or traversing (when not in air)
		UPrimitiveComponent* Base = InPawn->CharacterMoverComponent->GetMovementBase();
		if (IsValid(Base) && InPawn->CharacterMoverComponent->IsOnGround())
		{
			ProposedMove.LinearVelocity = Base->GetPhysicsLinearVelocity(NAME_None);
			ProposedMove.AngularVelocityDegrees = Base->GetPhysicsAngularVelocityInDegrees(NAME_None);
		}
		else
		{
			ProposedMove.LinearVelocity = InPawn->CharacterMoverComponent->GetVelocity();
			ProposedMove.AngularVelocityDegrees = FVector::ZeroVector;
		}
		ProposedMove.bHasDirIntent = true;
		ProposedMove.MixMode = EMoveMixMode::AdditiveVelocity;

		//Take every Constructed holder to generate the move which will be applied
		JumpLayeredMove->GenerateMove(
			MoverTickStartData,
			InPawn->CharacterMoverComponent->GetLastTimeStep(),
			InPawn->CharacterMoverComponent,
			const_cast<UMoverBlackboard*>(InPawn->CharacterMoverComponent->GetSimBlackboard()),
			ProposedMove
			);

		//Return constructed JumpedLayeredMove
		return JumpLayeredMove; 
	}
	else
	{
		//Debug the components that return nullptr
		UE_LOG(LogTemp, Error,
			TEXT("%s: ConstructGrappleMove - Invalid components:\n")
			TEXT("  InPawn: %s\n")
			TEXT("  InPawn->CharacterMoverComponent: %s\n"),
    
			*StaticClass()->GetName(),
			InPawn ? *InPawn->GetName() : TEXT("nullptr"),
			(InPawn && InPawn->CharacterMoverComponent) ? *InPawn->CharacterMoverComponent->GetName() : TEXT("nullptr")
		);
		return nullptr;
	}
}

void AGrappleSocket::EnableGrappling() const
{
	//Disabling bAttachedEnd will cause the physics of the Grapple Rope to be enabled, swinging the pawn
	if (IsValid(GrappleRope) && IsValid(Cached_PlayerPawn))
	{
		GrappleRope->bAttachEnd = false;
		//Attach Pawn to socket and apply its animation
		AttachPawnToGrappleSocket(Cached_PlayerPawn);
	
		//Set it so the Pawn is centered and in place on the edge of the Grapple Rope
		Cached_PlayerPawn->SetActorRelativeLocation(FVector::ZeroVector);
	}
}