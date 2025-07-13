#ifndef ARDTABLEMETHOD_H
#define ARDTABLEMETHOD_H

extern int BSFCTable[20][11];
extern float tableRowFeature[11];
extern int tableColumnFeature[20];

float calculateLinearInterpolation(float x, float x1, float x2, float y1, float y2);
float calculateBSFC(float engineSpeed, float engineTorque);

#endif