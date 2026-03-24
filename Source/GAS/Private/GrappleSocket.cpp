// Fill out your copyright notice in the Description page of Project Settings.
#include "GrappleSocket.h"
#include "GrapplingSocketWidgetComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "MoverPawn.h"
#include "CableComponent.h"
#include "Components/CapsuleComponent.h"

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
	APawn* PlayerSearch = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (IsValid(PlayerSearch)) Cached_PlayerPawn = Cast<AMoverPawn>(PlayerSearch);
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s:BeginPlay - Cached_PlayerPawn not found"), *StaticClass()->GetName());
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
	const APlayerController* PC = Cast<APlayerController>(Cached_PlayerPawn->GetController());
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

	//Bind so every time the player moves, it should update the widget
	Input->BindAction(Cached_PlayerPawn->GetMoveAction(), ETriggerEvent::Triggered, this, &AGrappleSocket::UpdateWidget);

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

void AGrappleSocket::UpdateWidget()
{
	if(!IsValid(Cached_PlayerPawn) || !IsValid(GrapplingSocketWidgetComponent) || !IsValid(Cached_Camera) || !IsValid(StaticMeshComponent)) return;
	if(IsInRangeToGrapple(Cached_PlayerPawn))
	{
		const FVector Direction = (Cached_PlayerPawn->GetActorLocation() -  GetActorLocation()).GetSafeNormal();
		const FBoxSphereBounds Bounds = StaticMeshComponent->GetStaticMesh()->GetBounds();
		const float Radius = Bounds.SphereRadius;
		const FVector NewLocation = GetActorLocation() + Direction * Radius;
		GrapplingSocketWidgetComponent->SetWorldLocation(NewLocation);
		
		GrapplingSocketWidgetComponent->SetWorldRotation((Cached_Camera->GetComponentLocation() - GrapplingSocketWidgetComponent->GetComponentLocation()).Rotation());
		GrapplingSocketWidgetComponent->SetWidgetVisibility(true);
	}
	else
	{
		GrapplingSocketWidgetComponent->SetWidgetVisibility(false);
	}
}

bool AGrappleSocket::IsInRangeToGrapple(const APawn* InPawn) const
{
	const float Dist = GetDistanceFromAPawn(InPawn);
	return Dist <= MinDistanceToGrapple;
}

float AGrappleSocket::GetDistanceFromAPawn(const APawn* InPawn) const
{
	FVector ToInteractionLocation = GetActorLocation() - InPawn->GetActorLocation();
	ToInteractionLocation.Z = 0.0f; // ignore vertical difference
	return ToInteractionLocation.Size();
}

void AGrappleSocket::AttachToGrappleSocket(APawn* InPawn)
{
	GrapplingSocketWidgetComponent->SetWidgetVisibility(false);
	if(GEngine)
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Grappled onto %s"), *this->GetName()));
	}
}

void AGrappleSocket::DetachFromGrappleSocket(APawn* InPawn)
{
	GrapplingSocketWidgetComponent->SetWidgetVisibility(true);
	if(GEngine)
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Grappled off from %s"), *this->GetName()));
	}
}

