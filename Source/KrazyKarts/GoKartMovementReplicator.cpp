// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"

#include "UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);


}

// Called when the game starts
void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

		//Instantiates our movement component.
	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}

// Called every frame
void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ensure(MovementComponent != nullptr))
		return;

	FGoKartMove LastMove = MovementComponent->GetLastMove();

	if (GetOwner()->Role == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(LastMove);
		//Moves client on server, client calls Server_SendMove -> Makes RPC to Server_SendMove_Implementation on server. TODO: add HasAuthority?
		Server_SendMove(LastMove);
	}

	//We are other clients, simulating on client.
	if (GetOwner()->Role == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}

	//TODO: Move GetRemoteRole out of Tick.
	//We are the server and in control of the pawn.
	if (GetOwner()->Role == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(LastMove);
	}
}

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

		//TODO: Break down Lerp and Slerp into separate functions.

		//Need to have atleast 2 updates before we start Lerping.
	//Compare to const small float since comparing float to 0 can be inaccurate. Since floating point numbers are "floating".
	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER)
	{
		return;
	}

		//TODO: Move and do once.
	if (!ensure(MovementComponent != nullptr))
		return;

		//Lerp
	FVector TargetLocation = ServerState.Transform.GetLocation();
	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	FVector StartLocation = ClientStartTransform.GetLocation();

	FVector NewLocation = FMath::LerpStable(StartLocation, TargetLocation, LerpRatio);

	//GetOwner()->SetActorLocation(NewLocation);

		//Slerp
	FQuat StartRotation = ClientStartTransform.GetRotation();
	FQuat TargetRotation = ServerState.Transform.GetRotation();

	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	GetOwner()->SetActorRotation(NewRotation);

		//CubicInterpolation

	//*100 to convert from UE4 cm to Meters.
	float VelocityToDerivative = ClientTimeBetweenLastUpdates * 100;

	FVector StartDerivative = ClientStartVelocity * VelocityToDerivative;
	FVector TargetDerivative = ServerState.Velocity * VelocityToDerivative;

	FVector NewDerivative = FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative;
	MovementComponent->SetVelocity(NewVelocity);

	FVector NewCubicLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);

	GetOwner()->SetActorLocation(NewCubicLocation);
}

void UGoKartMovementReplicator::CubicInterpolation()
{
	/*
	Slope = Derivative;

	= DeltaLocationn /DeltaAlpha.

	Velocity = Deltalocation /DeltaTime.
	DeltaAlpha = DeltaTime / Timebetweenlastupdates.
	
	Derivative = Velocity * TimeBetweenLastUpdates
	
	*/

	

}

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{

	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}

void UGoKartMovementReplicator::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		OnRep_Autonomous_ServerState();
		break;

	case ROLE_SimulatedProxy:
		OnRep_SimulatedProxy_ServerState();
		break;

	default:
		break;
	}
}

void UGoKartMovementReplicator::OnRep_SimulatedProxy_ServerState()
{
	if (!ensure(MovementComponent!= nullptr))
		return;

		//We are other clients simulating on client.
	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	ClientStartTransform = GetOwner()->GetActorTransform();
	ClientStartVelocity = MovementComponent->GetVelocity();
}

void UGoKartMovementReplicator::OnRep_Autonomous_ServerState()
{
		//We are Client.
	if (!ensure(MovementComponent != nullptr))
		return;

	//Sets/overrides values from server on client.
	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	//Clears Moves that have been acknowledged.
	ClearAcknowledgedMoves(ServerState.LastMove);

	//all moves that haven't been acknowledged, simulate.
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!ensure(MovementComponent != nullptr))
		return;

	MovementComponent->SimulateMove(Move);
	UpdateServerState(Move);
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)
{
	if (!ensure(MovementComponent != nullptr))
		return false;
	//TODO: Make better validation.
	return true;
}

void UGoKartMovementReplicator::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}
	UnacknowledgedMoves = NewMoves;
}