#ifndef ARDMATHFUN_H
#define ARDMATHFUN_H

const float aeroCoefficient = 0.279f;
const int vehicleMass = 206;
const float tireRadius = 0.2561f;
const int finalRatio = 10;
const float RPM2radconvertCoefficient = 0.1047f;
const int sec2hrconvertCoefficient = 3600;
const int W2kWconvertCoefficient = 1000;
const int g2LconvertCoefficient = 750;

float Fun_d(float *preinput, float *input, float *elapsed_time);
float FC_L(float *E_s, float *Tor, float *bsfc);
float T_Engine(float *preVehicle_speed_ms, float *Vehicle_speed_ms, float *elapsed_time);

#endif