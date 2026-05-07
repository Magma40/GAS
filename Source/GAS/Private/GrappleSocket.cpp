// Fill out your copyright notice in the Description page of Project Settings.
#include "GrappleSocket.h"
#include "GrapplingSocketWidgetComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "MoverPawn.h"
#include "CableComponent.h"
#include "GrapplerComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DefaultMovementSet/LayeredMoves/LaunchMove.h"

AGrappleSocket::AGrappleSocket()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Construct Root Component
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
	//Construct A StaticMesh
	StaticMeshComponent= CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	StaticMeshComponent->SetupAttachment(Root);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetHiddenInGame(false);
	
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
	 	return;
	 }

	//Construct SocketWidget
	GrapplingSocketWidgetComponent = CreateDefaultSubobject<UGrapplingSocketWidgetComponent>(TEXT("GrapplingSocketWidgetComponent"));
	GrapplingSocketWidgetComponent->SetupAttachment(Root);

	//Construct GrappleRope
	GrappleRope = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
	GrappleRope->SetupAttachment(Root);

	//Construct GrappleEdgeComponent
	GrappleEdgeComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("GrappleCapsuleComponent"));
	GrappleEdgeComponent->SetupAttachment(GrappleRope, "CableEnd");

	//Construct GrappleAreaComponent
	GrappleAreaComponent = CreateDefaultSubobject<USphereComponent>(TEXT("GrappleAreaComponent"));
	GrappleAreaComponent->SetupAttachment(Root);
	GrappleAreaComponent->InitSphereRadius(MinDistanceToGrapple);
	GrappleAreaComponent->SetCollisionProfileName(TEXT("Trigger"));	
}

void AGrappleSocket::BeginPlay()
{
	Super::BeginPlay();

	//Bind overlap to GrappleAreaComponent
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

	//Disable visibility on GrappleRope
	if (IsValid(GrappleRope))
	{
		GrappleRope->SetVisibility(false);
	}
}

void AGrappleSocket::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//Safety check
	if (!IsValid(GrapplingSocketWidgetComponent))
	{
		return;
	}

	/*
	 *Since GrapplingSocketWidgetComponent is done in C++ and not blueprint
	 *we cannot access and set visibility outside of C++, this is created to act as
	 *a check to see if either bEnableImage or bEnableText was changed on this Grapple Socket blueprint
	 */
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AGrappleSocket, bEnableImage))
	{
		if (bEnableImage)
		{
			GrapplingSocketWidgetComponent->SetWidgetImageVisibility(true);
		}
		else
		{
			GrapplingSocketWidgetComponent->SetWidgetImageVisibility(false);
		}
		
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AGrappleSocket, bEnableText))
	{
		if (bEnableText)
		{
			GrapplingSocketWidgetComponent->SetWidgetTextVisibility(true);
		}
		else
		{
			GrapplingSocketWidgetComponent->SetWidgetTextVisibility(false);
		}
	}
}

void AGrappleSocket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);;
	
	if(!IsValid(Cached_PlayerPawn))
	{
		return;
	}

	//If Pawn is in range but not grappling
	if (!Cached_PlayerPawn->GrapplerComponent->GetDoingGrapplingAction())
	{
		ConstructGrappleRope(Cached_PlayerPawn);
		UpdateWidget();
		return;
	}
	
	//If Pawn is not in range or is breaking the rope physics, detach it from the Grapple Socket
	if (GetDistanceFromAPawn(Cached_PlayerPawn) > MaxGrapplerRopeLength)
	{
		Cached_PlayerPawn->GrapplerComponent->DetachFromGrappleSocket(Cached_PlayerPawn);
		return;
	}

	//If Pawn is not on ground but is grappling, enable physics and swing pawn
	if (Cached_PlayerPawn->CharacterMoverComponent->IsAirborne() && Cached_PlayerPawn->GrapplerComponent->GetDoingGrapplingAction())
	{
		EnableGrappling();
		return;
	}

	FVector SocketLocation = Cached_PlayerPawn->SkeletalMeshComponent->GetSocketLocation("hand_rSocket");
	const FVector HandSocketRelativeLocationToGrappleSocket = GetActorTransform().InverseTransformPosition(SocketLocation);
	
	//Set the Grapple Rope End Location to the relative position of Pawn's hand to the Grapple Socket
	GrappleRope->EndLocation = HandSocketRelativeLocationToGrappleSocket;
}

void AGrappleSocket::UpdateWidget() const 
{
	if(!IsValid(Cached_PlayerPawn) || !IsValid(Cached_Camera))
	{
		return;
	}
	
	//If Pawn is in the range to grapple, display the widget
	if(!Cached_PlayerPawn->GrapplerComponent->GetDoingGrapplingAction() )
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

bool AGrappleSocket::IsInRangeToGrapple(const AMoverPawn* InPawn) const
{
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:IsInRangeToGrapple - InPawn not valid"), *StaticClass()->GetName());
		return false;
	}
	
	//Distance check from pawn to GrappleSocket
	const float Dist = GetDistanceFromAPawn(InPawn);
	return Dist <= MinDistanceToGrapple;
}

float AGrappleSocket::GetDistanceFromAPawn(const AMoverPawn* InPawn) const
{
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:GetDistanceFromAPawn - InPawn not valid"), *StaticClass()->GetName());
		return MAX_FLT;
	}
	
	//Return distance between pawn to GrappleSocket
	FVector ToInteractionLocation = GetActorLocation() - InPawn->GetActorLocation();
	ToInteractionLocation.Z = 0.0f; // ignore vertical difference
	return ToInteractionLocation.Size();
}

void AGrappleSocket::AttachToGrappleSocket(const AMoverPawn* InPawn)
{
	//Check first if pawn is valid
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:AttachToGrappleSocket - InPawn not valid"), *StaticClass()->GetName());
		return;
	}
	
	//Try to get the Pawn's Player Controller
	const APlayerController* PC = Cast<APlayerController>(InPawn->GetController());
	if (IsValid(PC))
	{
		UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent);
		if (IsValid(Input))
		{
			//Bind so player can swing around and apply force to Grapple Rope using WASD while grappling
			const FEnhancedInputActionEventBinding& Binding = Input->BindAction(InPawn->MoveAction, ETriggerEvent::Triggered,this, &AGrappleSocket::ApplyForceToGrappleMotion);
			BindingHandle = Binding.GetHandle();
		}
	}
	
	//Set that Pawn is currently doing Grappling Action
	InPawn->GrapplerComponent->SetDoingGrapplingAction(true);

	//Disable the widgets for the pawn while they are grappling
	GrapplingSocketWidgetComponent->SetWidgetVisibility(false);

	//When Attaching, show the already constructed Grapple Rope
	GrappleRope->SetVisibility(true);

	//Finalize by debugging successful attachement to Grapple Socket
	if(IsValid(GEngine) && EnableDebuggingText && EnableGrappleOntoDebuggingText)
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Grappled onto %s"), *this->GetName()));
	}
}

void AGrappleSocket::AttachPawnToGrappleSocket(AMoverPawn* InPawn) const
{
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:AttachPawnToGrappleSocket - InPawn not valid"), *StaticClass()->GetName());
		return;
	}
	
	//Attaches the Pawn (CapsuleComponent = RootComponent), to the end part of the rope
	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	InPawn->AttachToComponent(GrappleEdgeComponent, AttachmentRules);

	//Add the GameplayTag IsGrappling to enable animations
	//This also changes the current MoverComponent movement mode (IsGrappling = Flying)
	InPawn->CharacterMoverComponent->AddGameplayTag(Mover_IsGrappling);
	

}

void AGrappleSocket::DetachFromGrappleSocket(AMoverPawn* InPawn, const bool bApplyForce) const
{
	//Check first if pawn is valid
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:DetachFromGrappleSocket - InPawn not valid"), *StaticClass()->GetName());
		return;
	}

	//Try to get the Pawn's Player Controller
	const APlayerController* PC = Cast<APlayerController>(InPawn->GetController());
	if (IsValid(PC))
	{
		UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent);
		if (IsValid(Input))
		{
			//UnBind so player cannot swing around and apply force to Grapple Rope using WASD while grappling
			Input->RemoveBindingByHandle(BindingHandle);
		}
	}
	
	//Set that Pawn is currently not doing Grappling Action anymore
	InPawn->GrapplerComponent->SetDoingGrapplingAction(false);

	//Deference the Current Grapple Socket inside Pawns Grapple Component
	InPawn->GrapplerComponent->SetCurrentGrappleSocket(nullptr);

	//Enable the widgets for the pawn while they are not grappling anymore
	GrapplingSocketWidgetComponent->SetWidgetVisibility(true);
	
	//Detaches the Pawn from the Grapple Socket
	//GrappleRope->bAttachEnd = false;
	const FDetachmentTransformRules AttachmentRules(FDetachmentTransformRules::KeepWorldTransform);
	InPawn->DetachFromActor(AttachmentRules);

	//Removes the GameplayTag IsGrappling to disable animations
	//This also changes the current MoverComponent movement mode (IsGrappling = Flying)
	InPawn->CharacterMoverComponent->RemoveGameplayTag(Mover_IsGrappling);

	if (bApplyForce)
	{
		const FVector CurrentVelocity = InPawn->CharacterMoverComponent->GetVelocity();

		//Get camera direction
		FVector CameraLocation;
		FRotator CameraRotation;
		InPawn->GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

		const FVector CameraForward = CameraRotation.Vector();

		//Flatten for more controlled launches
		const FVector CameraForwardFlat = FVector(CameraForward.X, CameraForward.Y, 0.f).GetSafeNormal();

		//Forces
		const float ForwardBoost = 600.0f;
		const float UpwardBoost  = 300.0f;

		// Calculate velocity that the launch will produce
		FVector LaunchVelocity = CurrentVelocity + (CameraForwardFlat * ForwardBoost) + FVector(0.f, 0.f, UpwardBoost);

		//Make sure launch never feels too weak
		if (LaunchVelocity.Z < 200.f)
		{
			LaunchVelocity.Z = 200.f;
		}
		
		//Construct the JumpLayeredMove and all the necessary holders and variables
		//FLayeredMove's are forces which can be applied onto the Mover Component (adding a layer of force onto the movement)
		//This will cause it to not break the Mover Component movement and its animations
		//The LayeredMove also has a lifetime so its limited applied force
		TSharedPtr<FLayeredMove_Launch> JumpLayeredMove = ConstructGrappleMove(InPawn, LaunchVelocity);

		if (JumpLayeredMove)
		{
			//Queue the newly constructed JumpLayeredMove into the Movement
			InPawn->CharacterMoverComponent->QueueLayeredMove(JumpLayeredMove);
		}	
	}
	
	//Disable the visibly of the rope
	GrappleRope->SetVisibility(false);

	//Finalize by debugging successful detach from Grapple Socket
	if(IsValid(GEngine) && EnableDebuggingText && EnableGrappleOntoDebuggingText)
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Grappled off from %s"), *this->GetName()));
	}
}

TSharedPtr<FLayeredMove_Launch> AGrappleSocket::ConstructGrappleMove(const AMoverPawn* InPawn, const FVector& LaunchVelocity)
{
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:ConstructGrappleMove - InPawn not valid"), *StaticClass()->GetName());
		return nullptr;
	}
	
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

void AGrappleSocket::EnableGrappling() const
{
	//Disabling bAttachedEnd will cause the physics of the Grapple Rope to be enabled, swinging the pawn
	if (IsValid(Cached_PlayerPawn))
	{
		//Attach Pawn to socket and apply its animation
		AttachPawnToGrappleSocket(Cached_PlayerPawn);

		GrappleRope->bAttachEnd = false;

		//Set it so the Pawn is centered and in place on the edge of the Grapple Rope
		FVector SocketLocation = GrappleRope->GetSocketLocation("CableEnd");
		SocketLocation.Z += 88;
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
	//There are the same
	if (A.Equals(B))
	{
		return true;
	}

	//If vectors are within same range, there are the same
	const FVector AB = B - A;
	if (abs(AB.X) < Range &&
		abs(AB.Y) < Range &&
		abs(AB.X) < Range) return true;

	//They were not the same
	return false;
}

void AGrappleSocket::ApplyForceToGrappleMotion(const FInputActionValue& Value)
{
	if (!IsValid(Cached_PlayerPawn) || !IsValid(Cached_PlayerPawn->GetController()))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:ApplyForceToGrappleMotion - Cached_PlayerPawn or Cached_PlayerPawn was not valid"), *StaticClass()->GetName());
		return;
	}
	
	//WASD movement input
	FVector2D Input2D = Value.Get<FVector2D>();

	//Swing force for GrappleRope
	const float SwingForce = 100.0f;

	//Get camera rotation
	FVector CameraLocation;
	FRotator CameraRotation;
	Cached_PlayerPawn->GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

	//Ignore pitch so we don't push up/down unintentionally
	FRotator YawRotation(0.f, CameraRotation.Yaw, 0.f);

	//Get forward and right vectors of camera
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	//Calculate input to world-space force relative to camera
	FVector Force = (Forward * Input2D.Y +  Right   * Input2D.X) * SwingForce;

	//Apply new force to the CableRope
	GrappleRope->CableForce = Force;
}

void AGrappleSocket::ConstructGrappleRope(AMoverPawn* InPawn) const
{
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("%s:ConstructGrappleRope - InPawn not valid"), *StaticClass()->GetName());
		return;
	}
	
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

void AGrappleSocket::OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//Check if actor we overlapped with is valid
	if (IsValid(OtherActor))
	{
		//If pawn is valid, cache variables
		AMoverPawn* FoundPawn = Cast<AMoverPawn>(OtherActor);
		if (IsValid(FoundPawn) && IsValid(FoundPawn->GrapplerComponent) && !FoundPawn->GrapplerComponent->GetDoingGrapplingAction())
		{
			//Cache PlayerPawn
			Cached_PlayerPawn = FoundPawn;

			//If pawn has entered beforehand, no need to cache it again
			if (!IsValid(Cached_Camera))
			{
				//Cache Camera
				Cached_Camera = Cast<UCameraComponent>(FoundPawn->GetComponentByClass(UCameraComponent::StaticClass()));
				if (!IsValid(Cached_Camera))
				{
					UE_LOG(LogTemp, Error, TEXT("%s:OverlapBegin - Cached_Camera not found"), *StaticClass()->GetName());
					return;
				}
			}

			//Do this once before anything in case entering and clicking grappling is are very close in time to each-other
			ConstructGrappleRope(FoundPawn);
		}
	}
}

void AGrappleSocket::OverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//Check if actor we overlapped with is valid
	if (IsValid(OtherActor))
	{
		//If pawn is valid, dereference its variables
		AMoverPawn* FoundPawn = Cast<AMoverPawn>(OtherActor);
		if (IsValid(FoundPawn) && IsValid(FoundPawn->GrapplerComponent) && FoundPawn->GrapplerComponent->GetDoingGrapplingAction())
		{
			//If pawn was grappling but is now too far away from GrappleSocket
			FoundPawn->GrapplerComponent->DetachFromGrappleSocket(FoundPawn);
		}

		//Dereference pawn
		Cached_PlayerPawn = nullptr;
		
		if (IsValid(GrapplingSocketWidgetComponent))
		{
			//No need to show the Widget to Pawn if Pawn is not in range
			GrapplingSocketWidgetComponent->SetWidgetVisibility(false);	
		}
	}
}