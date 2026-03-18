// Fill out your copyright notice in the Description page of Project Settings.
#include "GrappleSocket.h"
#include "GrapplingSocketWidgetComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
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

	GrapplingSocketWidgetComponent  = CreateDefaultSubobject<UGrapplingSocketWidgetComponent>(TEXT("GrapplingSocketWidgetComponent"));
	GrapplingSocketWidgetComponent->SetupAttachment(Root);
}

// Called when the game starts or when spawned
void AGrappleSocket::BeginPlay()
{
	Super::BeginPlay();

	//Try to get the PlayerPawn in the world
	APawn* PlayerSearch = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (IsValid(PlayerSearch)) Cached_PlayerPawn = PlayerSearch;
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

// Called every frame
void AGrappleSocket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
	if(GEngine)
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Grappled onto %s"), *this->GetName()));
	}
}

void AGrappleSocket::DetachToGrappleSocket(APawn* InPawn)
{
	if(GEngine)
	{	
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Grappled off from %s"), *this->GetName()));
	}
}

