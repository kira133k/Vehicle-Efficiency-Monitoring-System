/*
Arduino version
*/
#include "ardmathfun.h"
#include <math.h>
#include <Arduino.h>

// Differential
float Fun_d(float *preinput, float *input, float *elapsed_time)
{
    float derivative = 0, previousValue = 0, currentValue = 0;
    previousValue = (*preinput), currentValue = (*input);
    derivative = (currentValue - previousValue) / (*elapsed_time);
    return derivative;
}

// Engine torque
float T_Engine(float *preVehicle_speed_ms, float *Vehicle_speed_ms, float *elapsed_time)
{
    float acceleration = 0.0, F_air = 0.0, F_rolling = 0.0, F_wheel = 0.0, T_wheel = 0.0, T_engine = 0.2;

    acceleration = Fun_d(preVehicle_speed_ms, Vehicle_speed_ms, elapsed_time);
    Serial.print("Acceleration= ");
    Serial.print(acceleration);
    Serial.println("  m/s^2");

    F_air = aeroCoefficient * (*Vehicle_speed_ms) * (*Vehicle_speed_ms);

    if ((*Vehicle_speed_ms) == 0)
    {
        F_rolling = 0.0;
    }
    else
    {
        F_rolling = 20.21;
    }

    F_wheel = (vehicleMass * acceleration) + F_rolling + F_air;
    /*
    Serial.print("F wheel= ");
    Serial.print(F_wheel);
    Serial.println(" N");

    T_wheel = F_wheel * tireRadius;
    Serial.print("T wheel= ");
    Serial.print(T_wheel);
    Serial.println(" N-m");*/

    T_engine = T_wheel / finalRatio;
    Serial.print("T engine= ");
    Serial.print(T_engine);
    Serial.println(" N-m");

    T_engine = fabsf(T_engine);

    /*
    torque = T_engine;
    if (torque > pretorque)
    {
        T_engine = torque + pretorque;
    }
    else
    {
        T_engine = torque - pretorque;
    }
    pretorque = torque;
    */

    // idle speed condition
    if ((*Vehicle_speed_ms) == 0)
    {
        T_engine = 1.11;
    }

    return T_engine;
}

// FC(L)
float FC_L(float *engineSpeed, float *engineTorque, float *BSFC)
{
    float engineSpeedinside = 0;
    double fc = 0, fcg = 0, fcl = 0;
    engineSpeedinside = (*engineSpeed) * RPM2radconvertCoefficient; // rpm to rad/sec
    fcg = engineSpeedinside * (*engineTorque) * (*BSFC) / (sec2hrconvertCoefficient * W2kWconvertCoefficient);
    Serial.print("FC(g)= ");
    Serial.println(fcg, 5);
    fcl = fcg / g2LconvertCoefficient;
    Serial.print("FC(L)= ");
    Serial.println(fcl, 5);
    return fcl;
}
