#ifndef MAINFUNCTION_H
#define MAINFUNCTION_H

#include <cstdint>

const float fluidDensity = 1.2f;         // 1.205 kg/m³ is replaced with 1.2 kg/m³ for simplicity.
const float dragCoefficient = 0.775f;    // This coefficient is determined by the properties of your test object.
const float vehicleArea = 0.6f;          // This coefficient is determined by the properties of your test object.
const uint8_t vehicleMass = 206U;        // This coefficient is determined by the properties of your test object.
const float tireRadius = 0.256f;         // This coefficient is determined by the properties of your test object.
const uint8_t transmissionRatio = 10U;   // This coefficient is determined by the properties of your test object.
const float transmissionEfficiency = 1U; // This coefficient is determined by the properties of your test object.

const uint16_t SECtoHR = 3600U;
const uint16_t mSECtoSEC = 1000U;
const uint16_t WattTOkiloWatt = 1000U;
const uint16_t gaslineDensity = 750U; // In this case, we use 92% isooctane gasoline with a density of 0.7573 g/mL at 15°C, which is simply approximated as 0.75 g/mL.

float calculateDerivative(float prevalue, float value, float elapsedTime);
float calculateEngineTorque(float preVehicleSpeed, float VehicleSpeed, float elapsedTime);
float calculateFuelConsumption(float engineSpeed, float engineTorque, float BSFC);

#endif