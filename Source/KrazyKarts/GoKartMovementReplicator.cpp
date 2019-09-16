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

	//TODO: use reference to movementcomponent to call.
	if (GetOwner()->Role == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		//simulate move locally. (As to not simulate move on server twice.)
		MovementComponent->SimulateMove(Move);
		GetUnacknowledgedMoves().Add(Move);
		//Moves client on server, client calls Server_SendMove -> Makes RPC to Server_SendMove_Implementation on server. TODO: add HasAuthority?
		Server_SendMove(Move);
	}

	//We are the client, simulated are other clients/server in game.
	if (GetOwner()->Role == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(GetServerState().LastMove);
	}

	//TODO: Move GetRemoteRole out of Tick.
	//We are the server and in control of the pawn.
	if (GetOwner()->Role == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}
}

TArray <FGoKartMove> UGoKartMovementReplicator::GetUnacknowledgedMoves() const
{
	return UnacknowledgedMoves;
}

FGoKartState UGoKartMovementReplicator::GetServerState() const
{
	return ServerState;
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}

void UGoKartMovementReplicator::OnRep_ServerState()
{
	if (!ensure(MovementComponent != nullptr))
		return;

	//Sets/overrides values from client on server.
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

	ServerState.LastMove = Move;

	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
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