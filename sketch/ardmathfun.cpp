/*
Author:Kira Chien, Department of Vehicle engineering, National Taipei University of Technology, Taiwan
Version:1.01
*/

#include "ardmathfun.h"
#include <math.h>
#include <Arduino.h>

float calculateDerivative(float, float, float);
float calculateEngineTorque(float, float, float);
float calculateFuelConsumption(float, float, float);

// Calculates the difference (rate of change)
// between the current value and the previous value, based on the elapsed time.
float calculateDerivative(float prevalue, float value, float elapsedTime)
{
    if (elapsedTime == 0)
    {
        elapsedTime = 0.1;
    }

    float derivative = (value - prevalue) / elapsedTime;
    return derivative;
}

// Calculates the engine torque
// use the current vehicle speed(unit: m/s) and the previous vehicle speed(unit: m/s), based on the elapsed time.
float calculateEngineTorque(float preVehicleSpeed, float VehicleSpeed, float elapsedTime)
{
    float rollingForce;

    if (elapsedTime == 0)
    {
        elapsedTime = 0.1;
    }

    if (VehicleSpeed == 0)
    {
        rollingForce = 0.0;
    }
    else
    {
        rollingForce = 20.21;
    }

    float acceleration = calculateDerivative(preVehicleSpeed, VehicleSpeed, elapsedTime);

    // Serial.print("Acceleration= ");
    // Serial.print(acceleration);
    // Serial.println("  m/s^2");

    float aerodynamicDrag = 0.5 * fluidDensity * dragCoefficient * vehicleArea * VehicleSpeed * VehicleSpeed; // 0.5 is the 1/2 factor from the aerodynamic drag equation.

    // Rolling force idle condition.

    float tireForce = (vehicleMass * acceleration) + rollingForce + aerodynamicDrag;

    // Serial.print("Tires Force= ");
    // Serial.print(tireForce);
    // Serial.println(" N");

    float tireTorque = tireForce * tireRadius;

    // Serial.print("Tires Torque= ");
    // Serial.print(tireTorque);
    // Serial.println(" N-m");

    float engineTorque = tireTorque / (transmissionRatio * transmissionEfficiency);

    // Serial.print("Engine Torque= ");
    // Serial.print(engineTorque);
    // Serial.println(" N-m");

    engineTorque = fabsf(engineTorque);

    // Vehice speed idle condition
    if (VehicleSpeed == 0)
    {
        engineTorque = 1.11;
    }

    return engineTorque;
}

// Calculates Fuel Consumption(unit:Liter)
// use engine speed, engine torque, BSFC value.
float calculateFuelConsumption(float engineSpeed, float engineTorque, float BSFC)
{
    double FuelConsumptionGram = engineSpeed * engineTorque * BSFC / (SECtoHR * WattTOkiloWatt);
    // Serial.print("Fuel Consumption= ");
    // Serial.print(FuelConsumptionGram, 5);
    // Serial.println(" (g)");

    double FuelConsumptionLiter = FuelConsumptionGram / GRAMtoLITER;
    // Serial.print("Fuel Consumption= ");
    // Serial.print(FuelConsumptionLiter, 5);
    // Serial.println(" (L)");

    return FuelConsumptionLiter;
}
