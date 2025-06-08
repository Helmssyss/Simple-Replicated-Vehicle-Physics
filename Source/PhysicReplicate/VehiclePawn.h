#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VehiclePawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UVehicleMovementComponent;

UCLASS()
class PHYSICREPLICATE_API AVehiclePawn : public APawn{
	public:
		AVehiclePawn();
		FORCEINLINE USkeletalMeshComponent* GetMesh() const {return MeshComponent;}
	
	private:
		GENERATED_BODY()

		virtual void BeginPlay() override;
		virtual void Tick(float DeltaTime) override;
		virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

		void MoveFB(float Value);
		void MoveRL(float Value);
		void LookRight(float Value);
		void LookUp(float Value);

		UPROPERTY(EditAnywhere,Category="Vehicle|Mesh")
		USkeletalMeshComponent* MeshComponent;

		UPROPERTY(EditAnywhere)
		USpringArmComponent* CameraBoom;

		UPROPERTY(EditAnywhere)
		UCameraComponent* CameraComponent;

		UPROPERTY(EditAnywhere,Category="MovementComponent")
		UVehicleMovementComponent* VehicleMovementComponent;
};
