#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>
#include "ardtablemethod.h"
#include "ardmathfun.h"
#include <string.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
const float RPMtoRAD = 0.1047f; // Vehicle engine speed converted from RPM to rad/sec
const float KMHRtoMS = 0.2778f; // Vehicle engine speed converted from RPM to rad/sec

#define SUPPORTED_PID "0100"
#define COOLANT_TEMPERATURE "0105"
#define ENIGNE_SPEED "010C"
#define VEHICLE_SPEED "010D"
#define THROTTLE_POSITION "0111"

enum CAN_PID
{
    SUPPORTPID,
    COOLANT,
    ENGINESPEED,
    VEHICLESPEED,
    THROTTLEPOSITION,
    NOPID
};

enum Unit
{
    NOUNIT,
    DEGREE,
    RPM,
    KMPH,
    ANGLE
};

enum Time
{
    halfSecond = 500,
    oneSecond = 1000,
    onehalfSecond = 1500,
    twoSecond = 2000,
    twohalfSecond = 2500,
    threeSecond = 3000
};

typedef struct
{
    char can_pid[7];
    char can_title[20];
    char can_message[20];
    char can_value[20];
} Vehiclemessage;

typedef struct
{
    int num;
    int coolant;
    int vehicleSpeed;
    int engineSpeed;
    int thresholdPosition;
    clock_t currentReciveTime;
    clock_t previousReciveTime;
} ReciveData;

TaskHandle_t InitializationHandle = NULL, AcquisitionHandle = NULL, processingHandle = NULL;
QueueHandle_t xDataQueue;

void ReadData(Vehiclemessage *, CAN_PID);
void showOnScreen(Vehiclemessage *, bool, Unit, clock_t);

void setup()
{
    lcd.init();
    lcd.backlight();
    lcd.begin(16, 2);
    lcd.noCursor();
    lcd.noBlink();
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    Serial2.setTimeout(600);

    while (!Serial)
    {
        Serial.println("Serial error");
    }
    while (!Serial2)
    {
        Serial.println("Serial2 error");
    }

    xDataQueue = xQueueCreate(5, sizeof(Vehiclemessage));

    while (xDataQueue == NULL)
    {
        xDataQueue = xQueueCreate(5, sizeof(Vehiclemessage));
    }

    xTaskCreate(InitializationTask, "Initialization", 2048, NULL, 1, &InitializationHandle);
    xTaskCreate(DataAcquisitionTask, "DataAcquisition", 4096, NULL, 2, &AcquisitionHandle);
    xTaskCreate(DataProcessingTask, "DataProcessing", 4096, NULL, 2, &processingHandle);

    xTaskNotifyGive(InitializationHandle);
}

void loop()
{
}

void InitializationTask(void *pvParameters)
{
    Vehiclemessage InitializationData;
    memset(&InitializationData, 0, sizeof(InitializationData));

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        lcd.setCursor(0, 0);
        lcd.print("Bootup Task         ");
        lcd.setCursor(0, 1);
        lcd.print("Creating......         ");
        vTaskDelay(500);

        strcpy(InitializationData.can_pid, "ATZ");
        ReadData(&InitializationData, NOPID);
        vTaskDelay(100);
        memset(&InitializationData, 0, sizeof(InitializationData));

        strcpy(InitializationData.can_pid, "ATSP0");
        ReadData(&InitializationData, NOPID);
        vTaskDelay(100);
        memset(&InitializationData, 0, sizeof(InitializationData));

        strcpy(InitializationData.can_pid, "ATE0");
        ReadData(&InitializationData, NOPID);
        vTaskDelay(100);
        memset(&InitializationData, 0, sizeof(InitializationData));

        strcpy(InitializationData.can_pid, "ATI");
        ReadData(&InitializationData, NOPID);
        vTaskDelay(100);

        if (strstr(InitializationData.can_message, "ELM327"))
        {
            lcd.clear();
            lcd.print("ELM327 connected!...              ");
            lcd.setCursor(0, 1);
            lcd.print("Connection OK                        ");
            vTaskDelay(1500);
            memset(&InitializationData, 0, sizeof(InitializationData));
            break;
        }
        else
        {
            lcd.clear();
            lcd.print("Connection Fail!!                 ");
            lcd.setCursor(0, 1);
            lcd.print("Retry Connection!                     ");
            vTaskDelay(1000);
            memset(&InitializationData, 0, sizeof(InitializationData));
            Serial.println("ATI Rebuild");
            xTaskNotifyGive(InitializationHandle);
        }
    }

    xTaskNotifyGive(InitializationHandle);

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        lcd.setCursor(0, 0);
        lcd.print("Connecting to          ");
        lcd.setCursor(0, 1);
        lcd.print("Vehicle....................");
        vTaskDelay(1000);

        strcpy(InitializationData.can_pid, SUPPORTED_PID);
        ReadData(&InitializationData, SUPPORTPID);
        showOnScreen(&InitializationData, false, NOUNIT, oneSecond);

        if (!strstr(InitializationData.can_message, "4100"))
        {
            lcd.clear();
            lcd.print("Vehicle connected!...              ");
            lcd.setCursor(0, 1);
            lcd.print("Connection OK                        ");
            vTaskDelay(1500);
            memset(&InitializationData, 0, sizeof(InitializationData));
            break;
        }
        else
        {
            lcd.clear();
            lcd.print("Connection Fail!!                 ");
            lcd.setCursor(0, 1);
            lcd.print("Retry Connection!                     ");
            vTaskDelay(1000);
            memset(&InitializationData, 0, sizeof(InitializationData));
            Serial.println("Support PID Rebuild");
            xTaskNotifyGive(InitializationHandle);
        }
    }
    Serial.println("Task Finished");
    xTaskNotifyGive(AcquisitionHandle);
    vTaskDelete(InitializationHandle);
}

void DataAcquisitionTask(void *pvParameters)
{
    CAN_PID state = COOLANT;
    Vehiclemessage AcquisitionData;
    ReciveData SendData;
    memset(&AcquisitionData, 0, sizeof(AcquisitionData));
    memset(&SendData, 0, sizeof(AcquisitionData));
    static clock_t currentTime;
    static clock_t previousTime;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (int step = 0; step < 4; step++)
        {
            previousTime = currentTime;
            currentTime = millis();
            switch (state)
            {
            case COOLANT:
                strcpy(AcquisitionData.can_pid, COOLANT_TEMPERATURE);
                ReadData(&AcquisitionData, COOLANT);
                SendData.coolant = atoi(AcquisitionData.can_value);
                SendData.currentReciveTime = currentTime;
                SendData.previousReciveTime = previousTime;
                Serial.println((String)AcquisitionData.can_value + "°C          ");
                showOnScreen(&AcquisitionData, true, DEGREE, oneSecond);
                memset(&AcquisitionData, 0, sizeof(AcquisitionData));
                state = ENGINESPEED;
                break;
            case ENGINESPEED:
                strcpy(AcquisitionData.can_pid, ENIGNE_SPEED);
                ReadData(&AcquisitionData, ENGINESPEED);
                SendData.engineSpeed = atoi(AcquisitionData.can_value);
                SendData.currentReciveTime = currentTime;
                SendData.previousReciveTime = previousTime;
                Serial.println((String)AcquisitionData.can_value + "RPM          ");
                showOnScreen(&AcquisitionData, true, RPM, oneSecond);
                memset(&AcquisitionData, 0, sizeof(AcquisitionData));
                state = VEHICLESPEED;
                break;
            case VEHICLESPEED:
                strcpy(AcquisitionData.can_pid, VEHICLE_SPEED);
                ReadData(&AcquisitionData, VEHICLESPEED);
                SendData.vehicleSpeed = atoi(AcquisitionData.can_value);
                SendData.currentReciveTime = currentTime;
                SendData.previousReciveTime = previousTime;
                Serial.println((String)AcquisitionData.can_value + "km/hr          ");
                showOnScreen(&AcquisitionData, true, KMPH, oneSecond);
                memset(&AcquisitionData, 0, sizeof(AcquisitionData));
                state = THROTTLEPOSITION;
                break;
            case THROTTLEPOSITION:
                strcpy(AcquisitionData.can_pid, THROTTLE_POSITION);
                ReadData(&AcquisitionData, THROTTLEPOSITION);
                SendData.vehicleSpeed = atoi(AcquisitionData.can_value);
                SendData.currentReciveTime = currentTime;
                SendData.previousReciveTime = previousTime;
                Serial.println((String)AcquisitionData.can_value + "°          ");
                showOnScreen(&AcquisitionData, true, ANGLE, oneSecond);
                memset(&AcquisitionData, 0, sizeof(AcquisitionData));
                state = COOLANT;
                break;
            }
        }
        xQueueSend(xDataQueue, &SendData, portMAX_DELAY);
        xTaskNotifyGive(processingHandle);
    }
}

void DataProcessingTask(void *pvParameters)
{
    static ReciveData calculateReciveData[10];

    static int i = 0, j = 0;
    static int totalExecutionTime;
    static float preVehicleSpeed, VehicleSpeed, ElapsedTime, preElapsedTime;
    static double totalFuelConsumption;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ReciveData reciveDataFormTask;

        if (xQueueReceive(xDataQueue, &reciveDataFormTask, portMAX_DELAY) == pdPASS)
        {

            if (i >= 10)
            {
                i = 0;
                j++;
            }
            calculateReciveData[i].num = i;
            calculateReciveData[i].coolant = reciveDataFormTask.coolant;
            calculateReciveData[i].vehicleSpeed = reciveDataFormTask.vehicleSpeed;
            calculateReciveData[i].engineSpeed = reciveDataFormTask.engineSpeed;
            calculateReciveData[i].thresholdPosition = reciveDataFormTask.thresholdPosition;
            clock_t reciveCurrentTime = reciveDataFormTask.currentReciveTime;
            clock_t recivePreviousTime = reciveDataFormTask.previousReciveTime;

            Serial.print("Index: ");
            Serial.println(calculateReciveData[i].num);
            Serial.print("Coolant: ");
            Serial.println(calculateReciveData[i].coolant);
            Serial.print("Vehicle Speed: ");
            Serial.println(calculateReciveData[i].vehicleSpeed);
            Serial.print("Engine Speed: ");
            Serial.println(calculateReciveData[i].engineSpeed);
            Serial.print("Threshold Position: ");
            Serial.println(calculateReciveData[i].thresholdPosition);

            float VehicleSpeed = calculateReciveData[i].vehicleSpeed;
            Serial.print("Vehicle Speed=");
            Serial.print(VehicleSpeed);
            Serial.println(" km/hr");

            VehicleSpeed = VehicleSpeed * KMHRtoMS;
            Serial.print("Vehicle Speed=");
            Serial.print(VehicleSpeed);
            Serial.println(" m/sec");

            float EngineSpeed = calculateReciveData[i].engineSpeed;

            // Engine speed idle condition
            if (VehicleSpeed <= 0)
            {
                EngineSpeed = 1000;
            }

            Serial.print("Engine Speed=");
            Serial.print(EngineSpeed);
            Serial.print(" RPM");

            EngineSpeed = EngineSpeed * RPMtoRAD;

            Serial.print(" = ");
            Serial.print(EngineSpeed);
            Serial.println(" rad/sec");

            ElapsedTime = (reciveCurrentTime - recivePreviousTime) / SECtoMIN;
            Serial.print("elapsed Time=");
            Serial.print(ElapsedTime);
            Serial.println(" sec");

            float Torque = calculateEngineTorque(preVehicleSpeed, VehicleSpeed, ElapsedTime);
            preVehicleSpeed = VehicleSpeed;

            Serial.print("Torque= ");
            Serial.print(Torque);
            Serial.println(" N-m");

            float BSFC = calculateBSFC(EngineSpeed, Torque);
            Serial.print("BSFC= ");
            Serial.print(BSFC);
            Serial.println(" g/kWh");

            if (isnan(BSFC) | BSFC <= 0)
            {
                Serial.print("    Error: BSFC is NaN or 0 at index ");
                Serial.print(i);
                Serial.println(" !\n");
                BSFC = 250.0;
            }

            double FuelConsumption = calculateFuelConsumption(EngineSpeed, Torque, BSFC);

            if (isnan(FuelConsumption) | FuelConsumption <= 0)
            {
                Serial.print("    Error: FCL is NaN or 0 at index ");
                Serial.print(i);
                Serial.println(" !\n");
                FuelConsumption = 0.0;
            }

            totalFuelConsumption += FuelConsumption;
            if (isnan(totalFuelConsumption) | totalFuelConsumption <= 0)
            {
                Serial.print("    Error: FCL total is NaN or 0 at index ");
                Serial.print(i);
                Serial.println(" !\n");
            }

            Serial.print("Total Fuel Consumption= ");
            Serial.print(totalFuelConsumption, 5);
            Serial.println(" (L)");

            Serial.print("This Round Execution time= ");
            Serial.print(ElapsedTime);
            Serial.println(" Sec");

            totalExecutionTime += ElapsedTime;
            Serial.print("Total Execution time= ");
            Serial.print(totalExecutionTime);
            Serial.println(" Sec");

            i++;

            lcd.clear();
            lcd.print("Total Fuel Consumption= ");
            lcd.setCursor(0, 1);
            lcd.print(totalFuelConsumption, 8);
            lcd.print("  L");
            vTaskDelay(1500);

            clock_t totalTime = millis() / SECtoMIN;
            uint8_t Hour = 0, Minute = 0;

            Serial.print("Total time= ");
            Serial.print(totalTime);
            Serial.println(" Sec");
            while (totalTime > 60)
            {
                Minute++;
                totalTime -= 60;
                while (Minute > 60)
                {
                    Hour++;
                    Minute -= 60;
                }
            }

            String timeDisplay = (String)Hour + "Hr " + (String)Minute + "Min " + (String)totalTime + "Sec ";
            Serial.print("Total time= ");
            Serial.println(timeDisplay);

            lcd.clear();
            lcd.print("Total Execution time=                ");
            lcd.setCursor(0, 1);
            lcd.print(timeDisplay);
            lcd.print("  Sec");
            vTaskDelay(1500);
        }
        Serial.print("Task processing over [");
        Serial.print(i);
        Serial.println("] time");
        xTaskNotifyGive(AcquisitionHandle);
        vTaskDelay(1000);
    }
}

void ReadData(Vehiclemessage *input, CAN_PID state)
{
    int Valuecalcu = 0, SvalueA = 0, SvalueB = 0;
    char Sbuffer[30] = {0}, filteredLine[30] = {0};
    String Svalue = "", temp1 = "", temp2 = "", Dvalue = "";
    String response = "";
    unsigned long timeout = millis() + 2000;

    Serial.write(input->can_pid);
    Serial.println();
    Serial2.write(input->can_pid);
    Serial2.write("\r\n");
    Serial2.flush();

    while (millis() < timeout)
    {
        if (Serial2.available() > 0)
        {
            response += Serial2.readString();
            break;
        }
        vTaskDelay(10);
    }

    if (response.length() > 0)
    {
        Svalue = response;
        Svalue.replace(">", "");
        Svalue.replace("OK", "");
        Svalue.replace("STOPPED", "");
        Svalue.replace("SEARCHING", "");
        Svalue.replace("NO DATA", "");
        Svalue.replace("?", "");
        Svalue.replace(",", "");
        Svalue.replace(".", "");
        Svalue.replace("\\", "");
        Svalue.replace(" ", "");
        Svalue.replace("\n", "");
        Svalue.replace("\r", "");
        Svalue.replace("\t", "");

        String pidremove = String(input->can_pid);
        Svalue.replace(pidremove, "");
    }

    Serial.print("Data from serial port{");
    Serial.println(response + "}");

    Serial.print("After Filter{");
    Serial.println(Svalue + "}");
    vTaskDelay(1000);

    SvalueA = strtol(Svalue.substring(4, 6).c_str(), NULL, 16);
    SvalueB = strtol(Svalue.substring(6, 8).c_str(), NULL, 16);

    switch (state)
    {

    case NOPID:
        Dvalue = Svalue;
        temp1 = Svalue + "                ";
        temp2 = Dvalue + "                ";

        if (strcmp(input->can_pid, "ATZ") == 0)
        {
            strlcpy(input->can_title, "ATZ setting", sizeof(input->can_title) - 1);
        }
        else if (strcmp(input->can_pid, "ATI") == 0)
        {
            strlcpy(input->can_title, "ATI setting", sizeof(input->can_title) - 1);
        }
        else if (strcmp(input->can_pid, "ATE0") == 0)
        {
            strlcpy(input->can_title, "ATE0 setting", sizeof(input->can_title) - 1);
        }
        else if (strcmp(input->can_pid, "ATSP0") == 0)
        {
            strlcpy(input->can_title, "ATSP0 setting", sizeof(input->can_title) - 1);
        }
        else
        {
            strlcpy(input->can_title, "No PID", sizeof(input->can_title) - 1);
        }

        strlcpy(input->can_message, temp1.c_str(), sizeof(input->can_message) - 1);
        strlcpy(input->can_value, temp2.c_str(), sizeof(input->can_value) - 1);

        break;

    case SUPPORTPID:
        strlcpy(input->can_title, "Sported PID:            ", sizeof(input->can_title) - 1);
        strlcpy(input->can_value, Svalue.c_str(), sizeof(input->can_value) - 1);
        strlcpy(input->can_message, Svalue.c_str(), sizeof(input->can_message) - 1);

        break;

    case COOLANT:
        Valuecalcu = SvalueA - 40;
        Dvalue = (String)Valuecalcu;

        temp1 = Svalue + "  ";
        temp2 = Dvalue + "  ";

        strlcpy(input->can_title, "0105 Coolant Temperature               ", sizeof(input->can_title) - 1);
        strlcpy(input->can_message, temp1.c_str(), sizeof(input->can_message) - 1);
        strlcpy(input->can_value, temp2.c_str(), sizeof(input->can_value) - 1);
        Serial.println("readdata case over");
        break;

    case ENGINESPEED:
        Valuecalcu = ((256 * SvalueA) + SvalueB) / 4;
        Dvalue = (String)Valuecalcu;

        temp1 = Svalue + "  ";
        temp2 = Dvalue + "  ";

        strlcpy(input->can_title, "010C Engine Speed               ", sizeof(input->can_title) - 1);
        strlcpy(input->can_message, temp1.c_str(), sizeof(input->can_message) - 1);
        strlcpy(input->can_value, temp2.c_str(), sizeof(input->can_value) - 1);
        Serial.println("readdata case over");
        break;

    case VEHICLESPEED:
        Valuecalcu = SvalueA;
        Dvalue = (String)Valuecalcu;

        temp1 = Svalue + "  ";
        temp2 = Dvalue + "  ";

        strlcpy(input->can_title, "010D Vehicle Speed               ", sizeof(input->can_title) - 1);
        strlcpy(input->can_message, temp1.c_str(), sizeof(input->can_message) - 1);
        strlcpy(input->can_value, temp2.c_str(), sizeof(input->can_value) - 1);
        Serial.println("readdata case over");

        break;

    case THROTTLEPOSITION:
        Valuecalcu = SvalueA * 100 / 255;
        Dvalue = (String)Valuecalcu;

        temp1 = Svalue + "  ";
        temp2 = Dvalue + "  ";

        strlcpy(input->can_title, "0111 Throttle Position               ", sizeof(input->can_title) - 1);
        strlcpy(input->can_message, temp1.c_str(), sizeof(input->can_message) - 1);
        strlcpy(input->can_value, temp2.c_str(), sizeof(input->can_value) - 1);
        Serial.println("readdata case over");

        break;

    default:

        temp1 = Svalue + "  ";
        temp2 = Dvalue + "  ";

        strlcpy(input->can_title, "Input Error               ", sizeof(input->can_title) - 1);
        strlcpy(input->can_message, temp1.c_str(), sizeof(input->can_message) - 1);
        strlcpy(input->can_value, temp2.c_str(), sizeof(input->can_value) - 1);

        break;
    }
}

void showOnScreen(Vehiclemessage *input, bool state, Unit unit, clock_t time)
{
    if (state == true)
    {
        lcd.clear();
        lcd.print((String)input->can_title + "          ");
        lcd.setCursor(0, 1);
        lcd.print((String)input->can_message + "          ");
        vTaskDelay(time);

        lcd.clear();
        lcd.print("Value is:                 ");
        Serial.println("Value is:                 ");
        lcd.setCursor(0, 1);
        switch (unit)
        {
        case DEGREE:
            lcd.print((String)input->can_value);
            lcd.write(0xDF);
            lcd.print("C          ");
            Serial.println((String)input->can_value + "°C          ");

            break;

        case RPM:
            lcd.print((String)input->can_value + "RPM          ");
            break;

        case KMPH:
            lcd.print((String)input->can_value + "km/hr          ");
            break;

        case ANGLE:
            lcd.print((String)input->can_value);
            lcd.write(0xDF);
            break;
        }

        vTaskDelay(time);
    }
    else
    {
        lcd.clear();
        lcd.print((String)input->can_title + "          ");
        lcd.setCursor(0, 1);
        lcd.print((String)input->can_message + "          ");
        vTaskDelay(time);
    }
}