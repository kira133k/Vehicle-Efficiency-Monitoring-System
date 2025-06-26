/*
Arduino version
*/
#include "ardtablemethod.h"
#include <Arduino.h>
#include <math.h>

//  BSFC 2D-table
int BSFC_Table[20][11]{
    5555, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    5555, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    5555, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    5555, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    5555, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    5555, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    5555, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    4848, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    3918, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    3253, 1070, 500, 440, 382, 357, 338, 320, 320, 340, 340,
    2755, 1130, 560, 430, 370, 342, 324, 310, 328, 330, 330,
    2367, 1210, 640, 430, 370, 344, 322, 307, 310, 320, 320,
    2057, 1280, 710, 440, 372, 350, 325, 305, 302, 317, 320,
    1803, 1120, 650, 455, 380, 352, 326, 306, 298, 305, 320,
    1592, 1080, 640, 470, 390, 356, 330, 310, 298, 310, 320,
    1413, 1000, 640, 485, 405, 363, 334, 314, 304, 320, 335,
    1259, 1040, 660, 485, 412, 381, 343, 325, 314, 335, 345,
    1126, 1090, 680, 490, 427, 383, 350, 333, 330, 370, 370,
    1010, 1180, 720, 510, 440, 380, 357, 343, 354, 378, 380,
    907, 1200, 740, 535, 454, 430, 400, 353, 380, 400, 400};

// Torque Nm
float tablecolumn[11]{0.118, 1.298, 2.478, 3.658, 4.838, 6.018, 7.198, 8.378, 9.558, 10.738, 11.800};

// Engine speed rad/sec
int tablerow[20]{
    4,
    45,
    86,
    127,
    168,
    168,
    209,
    262,
    314,
    367,
    419,
    471,
    524,
    576,
    628,
    681,
    733,
    785,
    838,
    890};

// top limit 250Nm
float torquein[]{
    0.2, 250};

float torquemap[]{
    0.2, 11};

// linearInterpolation
float linearInterpolation(float x, float x1, float x2, float y1, float y2)
{
    return y1 + ((x - x1) * (y2 - y1)) / (x2 - x1);
};

// Table method
float tablemethod(float *engineSpeed, float *engineTorque)
{
    int datahrow = 0, datalrow = 0, datahcolumn = 0, datalcolumn = 0;
    int datarowH[20] = {0}, datarowL[20] = {0}, datacolumnH[11] = {0}, datacolumnL[11] = {0};
    float Final = 0.0, targetrow[20] = {0};
    float engineTorqueinside = 0;

    Serial.print("Engine Speed = ");
    Serial.print(*engineSpeed);
    Serial.println(" rad/sec");

    Serial.print("Torque = ");
    Serial.print(*engineTorque);
    Serial.println(" N-m");

    engineTorqueinside = linearInterpolation(*engineTorque, torquein[0], torquein[1], torquemap[0], torquemap[1]);
    Serial.print("aftermapping = ");
    Serial.print(engineTorqueinside);
    Serial.println(" N-m");

    // Find the input vaule up and down limited in the Row
    for (int i = 0; tablerow[i] <= *engineSpeed; i++)
    {
        datahrow = i + 1;
        datalrow = i;
    }

    // Find the input vaule up and down limited in the Column
    for (int i = 0; tablecolumn[i] <= *engineTorque; i++)
    {
        datahcolumn = i + 1;
        datalcolumn = i;
    }

    /*
    cout << "P_Rowup = " << datahrow<<",";
    cout << "P_Rowdown = " << datalrow<<",";
    cout << "P_Cup = " << datahcolumn<<",";
    cout << "P_Cdown = " << datahcolumn<<endl<<endl;
    */

    for (int i = 0; i < 11; i++)
    {
        datarowH[i] = BSFC_Table[datahrow][i];
    }

    for (int i = 0; i < 11; i++)
    {
        datarowL[i] = BSFC_Table[datalrow][i];
    }

    // Targer row
    for (int n = 0; n < 11; n++)
    {
        targetrow[n] = linearInterpolation(*engineSpeed, tablerow[datalrow], tablerow[datahrow], BSFC_Table[datalrow][n], BSFC_Table[datahrow][n]);
    }

    Final = linearInterpolation(engineTorqueinside, tablecolumn[datalcolumn], tablecolumn[datahcolumn], targetrow[datalcolumn], targetrow[datahcolumn]);
    return Final;
};