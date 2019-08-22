// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"
#include "Runtime/Engine/Classes/GameFramework/SpringArmComponent.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime, FHitResult &HitResult)
{
	FVector Translation = Velocity * DeltaTime * 100; //multiplied by 100 to convert to meter from cm.

	AddActorWorldOffset(Translation, true, &HitResult);

	//If hit, reset velocity to 0.
	if (HitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}
// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	FVector Acceleration = Force / Mass;

	Velocity += Acceleration * DeltaTime;

	FHitResult HitResult;

	UpdateLocationFromVelocity(DeltaTime, HitResult);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
}

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
}
