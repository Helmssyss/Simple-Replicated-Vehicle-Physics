#include "VehicleMovementComponent.h"

#include "VehiclePawn.h"
#include "Net/UnrealNetwork.h"

#pragma region PhysicsTickFunction
void FVehiclePostPhysicsTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType,ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent){
	FActorComponentTickFunction::ExecuteTickHelper(
		Target, false, DeltaTime, TickType,
		[&](float DilatedTime){ Target->PostPhysicsTickComponent(DilatedTime, *this);}
	);
}

FName FVehiclePostPhysicsTickFunction::DiagnosticContext(bool bDetailed){
	if (bDetailed){
		return FName(*FString::Printf(TEXT("SkeletalMeshComponentClothTick/%s"), *GetFullNameSafe(Target)));
	}
	return FName(TEXT("SkeletalMeshComponentClothTick"));
}

FString FVehiclePostPhysicsTickFunction::DiagnosticMessage(){
	return Target->GetFullName() + TEXT("[UVehicleMovementComponent::PreClothTick]");
}
#pragma endregion

UVehicleMovementComponent::UVehicleMovementComponent(){
	PrimaryComponentTick.bCanEverTick = true;
	
	PostPhysicsTickFunction.bCanEverTick = true;
	PostPhysicsTickFunction.bStartWithTickEnabled = true;
	PostPhysicsTickFunction.SetTickFunctionEnable(true);
	PostPhysicsTickFunction.TickGroup = TG_PostPhysics;	
}

void UVehicleMovementComponent::RegisterComponentTickFunctions(bool bRegister){
	Super::RegisterComponentTickFunctions(bRegister);

	if (bRegister){
		if (SetupActorComponentTickFunction(&PostPhysicsTickFunction)){
			PostPhysicsTickFunction.Target = this;
			PostPhysicsTickFunction.AddPrerequisite(this, PrimaryComponentTick);
		}
	}else{
		if (PostPhysicsTickFunction.IsTickFunctionRegistered()){
			PostPhysicsTickFunction.UnRegisterTickFunction();
		}
	}
}

void UVehicleMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,Server_MovementData);
}

void UVehicleMovementComponent::PostPhysicsTickComponent(float DeltaTime,FVehiclePostPhysicsTickFunction& ThisTickFunction){
	if(!OwnerVehicle) return;

	FixedDeltaTime = DeltaTime;
	
	if(OwnerVehicle->GetLocalRole() > ROLE_SimulatedProxy && OwnerVehicle->IsLocallyControlled()){
		VehicleMove();
		const FVehicleMovementData MovementData = CreateMovementData();
		Server_SendMoveData(MovementData);
		UpdateHistory();
		
	}else{
		SimulatedMove();
	}
}

void UVehicleMovementComponent::SetThrottle(const float& Throttle_Input){
	VehicleInput.ThrottleInput = Throttle_Input;
}

void UVehicleMovementComponent::SetSteer(const float& Steer_Input){
	VehicleInput.SteerInput = Steer_Input;
}

void UVehicleMovementComponent::VehicleMove(){
	if(VehicleInput.ThrottleInput < 0.f){
		SetBrakeTorque(500.f,2);
		SetBrakeTorque(500.f,3);
	}else{
		SetBrakeTorque(0.f,2);
		SetBrakeTorque(0.f,3);
		SetDriveTorque(VehicleInput.ThrottleInput * 500.f,2);
		SetDriveTorque(VehicleInput.ThrottleInput * 500.f,3);	
	}

	SetSteerAngle(VehicleInput.SteerInput * 45.f,0);
	SetSteerAngle(VehicleInput.SteerInput * 45.f,1);
	
	if (OwnerVehicle->HasAuthority()){
		Multicast_SetSteerAngle(VehicleInput.SteerInput * 45.f);
	}else{
		Server_SetSteerAngle(VehicleInput.SteerInput * 45.f);
	}
}

void UVehicleMovementComponent::SimulatedMove() const{
	if(Simulated_MovementData.Location.IsZero()) return;

	const FVector AngularVelocity = FMath::VInterpTo(
		Simulated_MovementData.AngularVelocity,
		Server_MovementData.AngularVelocity,
		TimeDiffNormalize,FixedDeltaTime
	);

	const FVector ComponentVelocity = FMath::VInterpTo(
		Simulated_MovementData.ComponentVelocity,
		Server_MovementData.ComponentVelocity,
		TimeDiffNormalize, FixedDeltaTime
	);

	OwnerVehicle->GetMesh()->SetPhysicsLinearVelocity(ComponentVelocity);
	OwnerVehicle->GetMesh()->SetPhysicsAngularVelocityInDegrees(AngularVelocity);
	OwnerVehicle->GetMesh()->AddTorqueInDegrees(AngularVelocity, NAME_None, true);
}

void UVehicleMovementComponent::UpdateHistory(){
	if(MoveHistory.Num() <= 1){
		const FVehicleMovementData MoveData = CreateMovementData();
		MoveHistory.AddHead(MoveData);
	}else{
		float SnapshotHistoryLength = MoveHistory.GetHead()->GetValue().TimeStamp - MoveHistory.GetTail()->GetValue().TimeStamp;
		while (SnapshotHistoryLength > MaxRecordTime){
			MoveHistory.RemoveNode(MoveHistory.GetTail());
			SnapshotHistoryLength = MoveHistory.GetHead()->GetValue().TimeStamp - MoveHistory.GetTail()->GetValue().TimeStamp;
		}
		const FVehicleMovementData MoveData = CreateMovementData();
		MoveHistory.AddHead(MoveData);
	}
}

FVehicleMovementData UVehicleMovementComponent::CreateMovementData(const float DeltaTime) const{
	FVehicleMovementData MovementData;
	MovementData.Location = OwnerVehicle->GetMesh()->GetComponentLocation();
	MovementData.Rotation = OwnerVehicle->GetMesh()->GetComponentQuat();
	MovementData.AngularVelocity = OwnerVehicle->GetMesh()->GetPhysicsAngularVelocityInDegrees();
	MovementData.ComponentVelocity = OwnerVehicle->GetMesh()->GetComponentVelocity();
	MovementData.DeltaTime = DeltaTime > -1.f ? DeltaTime : FixedDeltaTime;
	MovementData.TimeStamp = GetWorld()->GetTimeSeconds();
	return  MovementData;
}


void UVehicleMovementComponent::Server_SendMoveData_Implementation(const FVehicleMovementData& MovementData){
	Server_MovementData = CreateMovementData();
	Multicast_SendMoveData(MovementData);
}

void UVehicleMovementComponent::Multicast_SendMoveData_Implementation(const FVehicleMovementData& MovementData){
	Simulated_MovementData = MovementData;
	UpdateHistory();
	
	const float HitTime = Server_MovementData.TimeStamp;
	const float OldestTime = MoveHistory.GetTail()->GetValue().TimeStamp;
	const float LatestTime = MoveHistory.GetHead()->GetValue().TimeStamp;
	const FVehicleMovementData *SelectedSnapshot;

	if (OldestTime > HitTime){return;}
		
	if (OldestTime == HitTime){SelectedSnapshot = &MoveHistory.GetTail()->GetValue();}
	
	else if (LatestTime <= HitTime){SelectedSnapshot = &MoveHistory.GetHead()->GetValue();}

	else{
		TDoubleLinkedList<FVehicleMovementData>::TDoubleLinkedListNode *CurrentNode = MoveHistory.GetHead();
		while (CurrentNode->GetValue().TimeStamp > HitTime){
			if (!CurrentNode->GetNextNode()) break;
			CurrentNode = CurrentNode->GetNextNode();
		}
		
		SelectedSnapshot = &CurrentNode->GetValue();
	}

	if (SelectedSnapshot){
		TimeDiffNormalize = FMath::Clamp((Server_MovementData.TimeStamp - SelectedSnapshot->TimeStamp) / (LatestTime - OldestTime), 0.f, 1.f);

		const FQuat NewRotation = FQuat::Slerp(
			SelectedSnapshot->Rotation,
			Server_MovementData.Rotation,
			TimeDiffNormalize * FixedDeltaTime
		);
			
		const FVector NewLocation = FMath::VInterpTo(
			SelectedSnapshot->Location,
			Server_MovementData.Location,
			TimeDiffNormalize, FixedDeltaTime
		);

		const FVector AngularVelocity = FMath::VInterpTo(
			SelectedSnapshot->AngularVelocity,
			Server_MovementData.AngularVelocity,
			TimeDiffNormalize,FixedDeltaTime
		);

		const FVector ComponentVelocity = FMath::VInterpTo(
			SelectedSnapshot->ComponentVelocity,
			Server_MovementData.ComponentVelocity,
			TimeDiffNormalize, FixedDeltaTime
		);

		OwnerVehicle->GetMesh()->SetWorldLocation(NewLocation,false,nullptr,ETeleportType::TeleportPhysics);
		OwnerVehicle->GetMesh()->SetWorldRotation(NewRotation,false,nullptr,ETeleportType::TeleportPhysics);
		OwnerVehicle->GetMesh()->SetPhysicsAngularVelocityInDegrees(AngularVelocity);
		OwnerVehicle->GetMesh()->SetPhysicsLinearVelocity(ComponentVelocity);
		
		if(OwnerVehicle->HasAuthority())
			Server_MovementData = CreateMovementData();
	}
}

void UVehicleMovementComponent::Server_SetSteerAngle_Implementation(const float& SteerAngle){
	Multicast_SetSteerAngle(SteerAngle);
}

void UVehicleMovementComponent::Multicast_SetSteerAngle_Implementation(const float& SteerAngle){
	SetSteerAngle(SteerAngle,0);
	SetSteerAngle(SteerAngle,1);
}
