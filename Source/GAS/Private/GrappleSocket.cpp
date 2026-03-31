// Fill out your copyright notice in the Description page of Project Settings.
#include "GrappleSocket.h"

#include <string>

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
	
	GrappleAreaComponent = CreateDefaultSubobject<USphereComponent>(TEXT("GrappleAreaComponent"));
	if (IsValid(GrappleAreaComponent))
	{
		GrappleAreaComponent->SetupAttachment(Root);
		GrappleAreaComponent->InitSphereRadius(MinDistanceToGrapple);
		GrappleAreaComponent->SetCollisionProfileName(TEXT("Trigger"));	
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: AGrappleSocket - GrappleAreaComponent is not valid"),*StaticClass()->GetName());
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
	AMoverPawn* PlayerSearch = Cast<AMoverPawn>(GetWorld()->GetFirstPlayerController()->GetPawn());
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
	if (IsValid(Input))
	{
		//Bind so every time the player moves, it should update the Widget
		Input->BindAction(PlayerSearch->GetMoveAction(), ETriggerEvent::Triggered, this, &AGrappleSocket::UpdateWidget);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - Input not found"), *StaticClass()->GetName());
		return;
	}

	if (IsValid(GrappleAreaComponent))
	{
		GrappleAreaComponent->OnComponentBeginOverlap.AddDynamic(this, &AGrappleSocket::OverlapBegin);
		GrappleAreaComponent->OnComponentEndOverlap.AddDynamic(this, &AGrappleSocket::OverlapEnd);	
	}

	//Apply the visibility if they are not correct
	if (IsValid(GrapplingSocketWidgetComponent))
	{
		GrapplingSocketWidgetComponent->SetWidgetImageVisibility(bEnableImage);
		GrapplingSocketWidgetComponent->SetWidgetTextVisibility(bEnableText);
		GrapplingSocketWidgetComponent->SetWidgetVisibility(false);	
	}

	if (IsValid(GrappleRope))
	{
		GrappleRope->SetVisibility(false);
	}
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
	Super::Tick(DeltaTime);;
	
	if(!IsValid(Cached_PlayerPawn)) return;

	if (IsValid(Cached_PlayerPawn->GrapplerComponent) && !Cached_PlayerPawn->GrapplerComponent->GetDoingGrapplingAction())
	{
		ConstructGrappleRope(Cached_PlayerPawn);
	}
	
	//If Pawn is not in range or is breaking the rope physics, detach it from the Grapple Socket
	if (IsValid(Cached_PlayerPawn->GrapplerComponent) && GetDistanceFromAPawn(Cached_PlayerPawn)  > MaxGrapplerRopeLength)
	{
		//Cached_PlayerPawn->GrapplerComponent->DetachFromGrappleSocket(Cached_PlayerPawn);
		//return;
	}
	
	if ((Cached_PlayerPawn->GetActorLocation() - Cached_PawnLocation).Size2D() > IntervalBetweenUpdatingPlayerLocations &&
		IsValid(GrapplingSocketWidgetComponent) &&
		!GrapplingSocketWidgetComponent->IsVisible())
	{
		Cached_PawnLocation = Cached_PlayerPawn->GetActorLocation();
	
		if (IsValid(Cached_PlayerPawn->CharacterMoverComponent) &&
			Cached_PlayerPawn->CharacterMoverComponent->IsAirborne() &&
			IsValid(Cached_PlayerPawn->GrapplerComponent) &&
			Cached_PlayerPawn->GrapplerComponent->GetDoingGrapplingAction())
		{
			EnableGrappling();
		}
		else if (IsValid(Cached_PlayerPawn->SkeletalMeshComponent) && IsValid(GrappleRope))
		{
			FVector SocketLocation = Cached_PlayerPawn->SkeletalMeshComponent->GetSocketLocation("hand_rSocket");
			const FVector HandSocketRelativeLocationToGrappleSocket = GetActorTransform().InverseTransformPosition(SocketLocation);
			
			//Set the Grapple Rope End Location to the relative position of Pawn's hand to the Grapple Socket
			GrappleRope->EndLocation = HandSocketRelativeLocationToGrappleSocket;
		}
	}
}

void AGrappleSocket::UpdateWidget()
{
	if(!IsValid(Cached_PlayerPawn) || !IsValid(GrapplingSocketWidgetComponent) || !IsValid(Cached_Camera) || !IsValid(StaticMeshComponent) && !IsValid(GrappleRope)) return;
	
	//If Pawn is in the range to grapple, display the widget
	if(!Cached_PlayerPawn->GrapplerComponent->GetDoingGrapplingAction() )
	{
		FVector SocketLocation = Cached_PlayerPawn->SkeletalMeshComponent->GetSocketLocation("hand_rSocket");
		const FVector HandSocketRelativeLocationToGrappleSocket = GetActorTransform().InverseTransformPosition(SocketLocation);
		 if (FVectorAlmostTheSame( GrappleRope->EndLocation,  HandSocketRelativeLocationToGrappleSocket, 10))
		 {
			//Calculate the location of the edge of the Grapple Socket mesh 
			const FVector Direction = (Cached_PlayerPawn->GetActorLocation() -  GetActorLocation()).GetSafeNormal();
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
	else
	{
		//No need to show the Widget to Pawn if Pawn is not in range
		GrapplingSocketWidgetComponent->SetWidgetVisibility(false);
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
	
	if(IsValid(GEngine) && EnableDebuggingText && EnableGrappleOntoDebuggingText)
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Grappled onto %s"), *this->GetName()));
	}

	if (IsValid(GrappleRope))
	{
		GrappleRope->SetVisibility(true);
	}
}

void AGrappleSocket::AttachPawnToGrappleSocket(AMoverPawn* InPawn) const
{
	if (IsValid(InPawn) && IsValid(InPawn->CapsuleComponent) && IsValid(GrappleEdgeComponent))
	{
		//Attaches the Pawn (CapsuleComponent = RootComponent), to the end part of the rope
		const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, false);
		InPawn->CapsuleComponent->AttachToComponent(GrappleEdgeComponent, AttachmentRules);

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
	return;
	if(IsValid(InPawn) && IsValid(InPawn->GrapplerComponent))
	{
		//Set that Pawn is currently not doing Grappling Action anymore
		InPawn->GrapplerComponent->SetDoingGrapplingAction(false);

		//Deference the Current Grapple Socket inside Pawns Grapple Component
		InPawn->GrapplerComponent->SetCurrentGrappleSocket(nullptr);

		//Reset Cached Pawn Location for when Pawn attaches again
		Cached_PawnLocation = FVector::ZeroVector;
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
	
	if(IsValid(GEngine) && EnableDebuggingText && EnableGrappleOntoDebuggingText)
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

		if(IsValid(GEngine) && EnableDebuggingText && EnableConstructRopeDebuggingText)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f,  FColor::Cyan, FString::Printf(
				TEXT("GrappleRope->EndLocation - %s  \n")
							TEXT("GrappleRope->CableLength - %f  \n"),
				
							*GrappleRope->EndLocation.ToString(),
							GrappleRope->CableLength));
		}
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

void AGrappleSocket::EnableGrappling()
{
	//Disabling bAttachedEnd will cause the physics of the Grapple Rope to be enabled, swinging the pawn
	if (IsValid(GrappleRope) && IsValid(Cached_PlayerPawn) && IsValid(GrappleEdgeComponent))
	{
		//Attach Pawn to socket and apply its animation
		AttachPawnToGrappleSocket(Cached_PlayerPawn);

		GrappleRope->bAttachEnd = false;

		//Set it so the Pawn is centered and in place on the edge of the Grapple Rope
		FVector SocketLocation = GrappleRope->GetSocketLocation("CableEnd");
		//FRotator  SocketRotation = GrappleRope->GetSocketRotation("CableEnd");
		Cached_PlayerPawn->SetActorLocation(SocketLocation);

		if(IsValid(GEngine) && EnableDebuggingText && EnableGrapplingActionDebuggingText)
		{
			GEngine->AddOnScreenDebugMessage(-1, -1.0f,  FColor::Cyan, FString::Printf(
				TEXT("Cached_PlayerPawn->WorldLocation - %s  \n")
							TEXT("Cached_PlayerPawn->WorldRotation - %s  \n")
							TEXT("Cached_PlayerPawn->CapsuleComponent->RelativeLocation - %s  \n")
							TEXT("Cached_PlayerPawn->CapsuleComponent->RelativeRotation - %s  \n"),
				
							*Cached_PlayerPawn->GetActorLocation().ToString(),
							*Cached_PlayerPawn->GetActorRotation().ToString(),
							*Cached_PlayerPawn->CapsuleComponent->GetRelativeLocation().ToString(),
							*Cached_PlayerPawn->CapsuleComponent->GetRelativeRotation().ToString()));
		}
	}
}

bool AGrappleSocket::FVectorAlmostTheSame(const FVector& A, const FVector& B, const float Range)
{
	if (A.Equals(B)) return true;

	const FVector AB = B - A;
	if (abs(AB.X) < Range &&
		abs(AB.Y) < Range &&
		abs(AB.X) < Range) return true;

	return false;
}

void AGrappleSocket::OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsValid(OtherActor))
	{
		AMoverPawn* FoundPawn = Cast<AMoverPawn>(OtherActor);
		if (IsValid(FoundPawn)) Cached_PlayerPawn = FoundPawn;
		if (IsValid(FoundPawn) && IsValid(FoundPawn->GrapplerComponent) && !FoundPawn->GrapplerComponent->GetDoingGrapplingAction())
		{
			ConstructGrappleRope(FoundPawn);
		}
	}
}

void AGrappleSocket::OverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsValid(OtherActor))
	{
		AMoverPawn* FoundPawn = Cast<AMoverPawn>(OtherActor);
		if (IsValid(FoundPawn) && IsValid(FoundPawn->GrapplerComponent) && FoundPawn->GrapplerComponent->GetDoingGrapplingAction())
		{
			FoundPawn->GrapplerComponent->DetachFromGrappleSocket(FoundPawn);
		}
		else
		{
			Cached_PlayerPawn = nullptr;
		}
		
		if (IsValid(GrapplingSocketWidgetComponent))
		{
			//No need to show the Widget to Pawn if Pawn is not in range
			GrapplingSocketWidgetComponent->SetWidgetVisibility(false);	
		}
	}
}