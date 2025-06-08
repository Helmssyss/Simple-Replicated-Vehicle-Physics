#pragma once

#include "CoreMinimal.h"
#include "SimpleWheeledVehicleMovementComponent.h"
#include "VehicleMovementComponent.generated.h"

class AVehiclePawn;

USTRUCT()
struct FVehiclePostPhysicsTickFunction : public  FTickFunction{
	GENERATED_BODY()

	UPROPERTY()
	class UVehicleMovementComponent* Target;
	
	virtual  void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	virtual FName DiagnosticContext(bool bDetailed) override;
	virtual FString DiagnosticMessage() override;
};

template<>
struct TStructOpsTypeTraits<FVehiclePostPhysicsTickFunction> : public TStructOpsTypeTraitsBase2<FVehiclePostPhysicsTickFunction>{
	enum{WithCopy = false};
};

USTRUCT()
struct FVehicleInput{
	GENERATED_BODY()

	UPROPERTY()
	float ThrottleInput;

	UPROPERTY()
	float SteerInput;

	FVehicleInput() : ThrottleInput(0.f), SteerInput(0.f){}
};

USTRUCT()
struct FVehicleMovementData{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector_NetQuantize100 Location;

	UPROPERTY()
	FVector_NetQuantize100 ComponentVelocity;

	UPROPERTY()
	FQuat Rotation;

	UPROPERTY()
	FVector_NetQuantize100 AngularVelocity;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float TimeStamp;

	bool operator==(const FVehicleMovementData& NewMove) const{
		return Location == NewMove.Location &&
			   Rotation == NewMove.Rotation &&
			   AngularVelocity == NewMove.AngularVelocity &&
			   DeltaTime == NewMove.DeltaTime &&
			   ComponentVelocity == NewMove.ComponentVelocity && 
			   TimeStamp == NewMove.TimeStamp;
	}

	bool operator !=(const FVehicleMovementData& NewMove) const{
		return !(*this == NewMove);
	}
};

UCLASS()
class PHYSICREPLICATE_API UVehicleMovementComponent : public USimpleWheeledVehicleMovementComponent{
	public:
		UVehicleMovementComponent();

		FORCEINLINE void SetOwner(AVehiclePawn* NewOwner) {OwnerVehicle = NewOwner;}

		void SetThrottle(const float& Throttle_Input);
		void SetSteer(const float& Steer_Input);
		
		friend FVehiclePostPhysicsTickFunction;
		
	private:
		GENERATED_BODY()

		virtual void RegisterComponentTickFunctions(bool bRegister) override;
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

		UFUNCTION(Server,Unreliable)
		void Server_SendMoveData(const FVehicleMovementData& MovementData);
		void Server_SendMoveData_Implementation(const FVehicleMovementData& MovementData);

		UFUNCTION(NetMulticast,Unreliable)
		void Multicast_SendMoveData(const FVehicleMovementData& MovementData);
		void Multicast_SendMoveData_Implementation(const FVehicleMovementData &MovementData);

		UFUNCTION(Server,Unreliable)
		void Server_SetSteerAngle(const float &SteerAngle);
		void Server_SetSteerAngle_Implementation(const float &SteerAngle);
	
		UFUNCTION(NetMulticast, Unreliable)
		void Multicast_SetSteerAngle(const float& SteerAngle);
		void Multicast_SetSteerAngle_Implementation(const float& SteerAngle);
	
		void PostPhysicsTickComponent(float DeltaTime, FVehiclePostPhysicsTickFunction& ThisTickFunction);
		void VehicleMove();
		void SimulatedMove() const;
		void UpdateHistory();
	
		FVehicleMovementData CreateMovementData(const float DeltaTime = -1.f) const;
	
		UPROPERTY()
		FVehiclePostPhysicsTickFunction PostPhysicsTickFunction;

		UPROPERTY()
		FVehicleInput VehicleInput;

		UPROPERTY()
		AVehiclePawn* OwnerVehicle;

		UPROPERTY()
		FVehicleMovementData Simulated_MovementData;

		UPROPERTY(Replicated)
		FVehicleMovementData Server_MovementData;

		TDoubleLinkedList<FVehicleMovementData> MoveHistory;
		const float MaxRecordTime = 4.f;
	
		float FixedDeltaTime;
		float TimeDiffNormalize;
};
