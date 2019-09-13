// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementComponent.h"

// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	//Conversion from UE units(cm) to meters.
	AccelerationDueToGravity = -GetWorld()->GetDefaultGravityZ() / 100;
}


// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->TimeSeconds;

	return Move;
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move)
{
	FVector ForwardForce = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	ForwardForce += GetAirResistance();
	ForwardForce += GetRollResistance();
	FVector Acceleration = ForwardForce / Mass;

	Velocity += Acceleration * Move.DeltaTime;
	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	FHitResult HitResult;

	UpdateLocationFromVelocity(Move.DeltaTime, HitResult);
}

FVector UGoKartMovementComponent::GetVelocity() const
{
	return Velocity;
}

void UGoKartMovementComponent::SetVelocity(FVector Value)
{
	 Velocity = Value;	
}

void UGoKartMovementComponent::SetThrottle(float Value)
{
	Throttle = Value;
}

void UGoKartMovementComponent::SetSteeringThrow(float Value)
{
	SteeringThrow = Value;
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime, FHitResult &HitResult)
{
	FVector Translation = Velocity * DeltaTime * 100; //multiplied by 100 to convert to meter from cm.

	GetOwner()->AddActorWorldOffset(Translation, true, &HitResult);

	//If hit, reset velocity to 0.
	if (HitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

FVector UGoKartMovementComponent::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector UGoKartMovementComponent::GetRollResistance()
{
	float NormalForce = Mass * AccelerationDueToGravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

void UGoKartMovementComponent::ApplyRotation(float DeltaTime, float SteeringThrow)
{
	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	GetOwner()->AddActorWorldRotation(RotationDelta, true);
}
