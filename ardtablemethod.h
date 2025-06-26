#ifndef ARDTABLEMETHOD_H
#define ARDTABLEMETHOD_H

extern int BSFC_Table[20][11];
extern int tablerow[20];
extern float tablecolumn[11];

float linearInterpolation(float x, float x1, float x2, float y1, float y2);
float tablemethod(float *engineSpeed, float *engineTorque);

#endif