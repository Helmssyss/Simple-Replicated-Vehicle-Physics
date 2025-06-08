#include "WheelFront.h"

UWheelFront::UWheelFront(){
	ShapeRadius = 35.0f;
	ShapeWidth = 10.0f;
	Mass = 20.0f;
	DampingRate = 0.25f;
	bAffectedByHandbrake = false;
	SteerAngle = 50.0;
	SuspensionMaxRaise = 5.0f;
	SuspensionMaxDrop = 25.0f;
	SuspensionNaturalFrequency = 7.0f;
	SuspensionDampingRatio = 1.0f;

	MaxBrakeTorque = 1500.0f;
	MaxHandBrakeTorque = 3000.0f;
}
