// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"
#include "Runtime/Engine/Classes/GameFramework/SpringArmComponent.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "Engine/World.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//enables replication on actor.
	bReplicates = true;
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	//Conversion from UE units(cm) to meters.
	AccelerationDueToGravity = -GetWorld()->GetDefaultGravityZ() / 100;

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}

}

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKart, ServerState);
}

void AGoKart::SimulateMove(FGoKartMove Move)
{
	FVector ForwardForce = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	ForwardForce += GetAirResistance();
	ForwardForce += GetRollResistance();
	FVector Acceleration = ForwardForce / Mass;

	Velocity += Acceleration * Move.DeltaTime;
	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	FHitResult HitResult;

	UpdateLocationFromVelocity(Move.DeltaTime, HitResult);
}

FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "ROLE_SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "ROLE_AutonomousProxy";
	case ROLE_Authority:
		return "ROLE_Authority";
	default:
		return "Error";
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		FGoKartMove Move;
		Move.DeltaTime = DeltaTime;
		Move.SteeringThrow = SteeringThrow;
		Move.Throttle = Throttle;
		//TODO: Set Move.Time;
		Server_SendMove(Move);

		if (!HasAuthority())
		{
			SimulateMove(Move);
		}
	}


	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(Role), this, FColor::White, DeltaTime);
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;
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

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}

FVector AGoKart::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollResistance()
{
	float NormalForce = Mass * AccelerationDueToGravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

void AGoKart::ApplyRotation(float DeltaTime, float SteeringThrow)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta, true);
}


// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
	//TODO: Add ServerState.LastMove.
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	//CLEAN one liner for checking that Value is within range -1 to 1.
	//return FMath::Abs(Value) <= 1;

		//TODO: Make better validation.
	return true;
}