#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>
#include <ardtablemethod.h>
#include <ardmathfun.h>
#include <string.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

using namespace std;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define PI 3.14159

#define SUPPORTED_PID "0100"
#define COOLANT_TEMPERATURE "0105"
#define ENIGNE_SPEED "010C"
#define VEHICLE_SPEED "010D"
#define THROTTLE_POSITION "0111"

enum CAN_PID
{
    S_PID,
    COOLANT,
    E_SPEED,
    V_SPEED,
    T_POSITION,
    NO_PID
};

enum Unit
{
    NOUNIT,
    DEGREE,
    RPM,
    KMPH,
    ANGLE
};

typedef struct
{
    uint8_t syncFlag;
    char can_pid[7]; // 0526不確定要寫0x0100還是0100但都要留多一個字元給\0
    char can_title[20];
    char can_message[20];
    char can_value[20];
} Vehiclemessage;

typedef struct
{
    int num;
    int coolant;
    int vehicleSpeed;
    int engingSpeed;
    int thresholdPosition;
} ReciveData;

TaskHandle_t bootupHandle = NULL, readdataHandle = NULL, dataprocessingHandle = NULL;
QueueHandle_t xDataQueue;

void ReadData(Vehiclemessage *, CAN_PID);
void showOnScreen(Vehiclemessage *, bool, Unit);

clock_t totalTime = millis();

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

    xDataQueue = xQueueCreate(5, sizeof(Vehiclemessage)); // 可塞5個Vehiclemessage

    if (xDataQueue == NULL)
    {
        // 錯誤處理
    }

    xTaskCreate(BootupTask, "BOOTUP", 4096, NULL, 1, &bootupHandle);
    xTaskCreate(ReadDataTask, "READDATA", 3072, NULL, 2, &readdataHandle);
    xTaskCreate(dataProcessingTask, "DISPLAY", 2048, NULL, 2, &dataprocessingHandle);

    xTaskNotifyGive(bootupHandle);
}

void loop()
{
    // 空實現，任務由FreeRTOS調度
}

void BootupTask(void *pvParameters)
{
    Vehiclemessage Bdata;
    memset(&Bdata, 0, sizeof(Bdata));

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        lcd.setCursor(0, 0);
        lcd.print("Bootup Task         ");
        lcd.setCursor(0, 1);
        lcd.print("Creating......         ");
        vTaskDelay(500);

        strcpy(Bdata.can_pid, "ATZ");
        ReadData(&Bdata, NO_PID);
        // showOnScreen(&Bdata, false, NOUNIT);
        vTaskDelay(100);
        memset(&Bdata, 0, sizeof(Bdata));

        strcpy(Bdata.can_pid, "ATSP0");
        ReadData(&Bdata, NO_PID);
        // showOnScreen(&Bdata, false, NOUNIT);
        vTaskDelay(100);
        memset(&Bdata, 0, sizeof(Bdata));

        strcpy(Bdata.can_pid, "ATE0");
        ReadData(&Bdata, NO_PID);
        // showOnScreen(&Bdata, false, NOUNIT);
        vTaskDelay(100);
        memset(&Bdata, 0, sizeof(Bdata));

        strcpy(Bdata.can_pid, "ATI");
        ReadData(&Bdata, NO_PID);
        // showOnScreen(&Bdata, false, NOUNIT);
        vTaskDelay(100);
        /*
        Serial.print("can_message is{");
        Serial.print(Bdata.can_message);
        Serial.println("}");*/

        if (strstr(Bdata.can_message, "ELM327"))
        {
            lcd.clear();
            lcd.print("ELM327 connected!...              ");
            lcd.setCursor(0, 1);
            lcd.print("Connection OK                        ");
            vTaskDelay(2000);
            memset(&Bdata, 0, sizeof(Bdata));
            break;
        }
        else
        {
            lcd.clear();
            lcd.print("Connection Fail!!                 ");
            lcd.setCursor(0, 1);
            lcd.print("Retry Connection!                     ");
            vTaskDelay(1500);
            memset(&Bdata, 0, sizeof(Bdata));
            Serial.println("ATI Rebuit");
            xTaskNotifyGive(bootupHandle);
        }
    }

    xTaskNotifyGive(bootupHandle);

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        lcd.setCursor(0, 0);
        lcd.print("Connecting to          ");
        lcd.setCursor(0, 1);
        lcd.print("Vehicle....................");
        vTaskDelay(1000);

        strcpy(Bdata.can_pid, SUPPORTED_PID);
        ReadData(&Bdata, S_PID);
        showOnScreen(&Bdata, false, NOUNIT);

        if (!strstr(Bdata.can_message, "4100"))
        {
            lcd.clear();
            lcd.print("Vehicle connected!...              ");
            lcd.setCursor(0, 1);
            lcd.print("Connection OK                        ");
            Serial.println("Support PID Success");
            vTaskDelay(2000);
            memset(&Bdata, 0, sizeof(Bdata));
            break;
        }
        else
        {
            lcd.clear();
            lcd.print("Connection Fail!!                 ");
            lcd.setCursor(0, 1);
            lcd.print("Retry Connection!                     ");
            vTaskDelay(1500);
            memset(&Bdata, 0, sizeof(Bdata));
            Serial.println("Support PID Rebuit");
            xTaskNotifyGive(bootupHandle);
        }
    }
    Serial.println("Task Finished");
    xTaskNotifyGive(readdataHandle);
    vTaskDelete(bootupHandle);
}

//  ====== ReadDataTask ======
void ReadDataTask(void *pvParameters)
{

    Vehiclemessage Rdata;
    CAN_PID state = COOLANT;
    ReciveData SendData;
    memset(&Rdata, 0, sizeof(Rdata));

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // 接setup或Display的通知，接到才開始Read data

        memset(&Rdata, 0, sizeof(Rdata));
        for (int step = 0; step < 4; step++)
        {
            switch (state)
            {
            case COOLANT:
                strcpy(Rdata.can_pid, COOLANT_TEMPERATURE);
                ReadData(&Rdata, COOLANT);
                SendData.coolant = atoi(Rdata.can_value);
                Serial.println("IN READDATA");
                Serial.println((String)Rdata.can_value + "°C          ");
                showOnScreen(&Rdata, true, DEGREE);
                memset(&Rdata, 0, sizeof(Rdata));
                state = E_SPEED;
                break;
            case E_SPEED:
                strcpy(Rdata.can_pid, ENIGNE_SPEED);
                ReadData(&Rdata, E_SPEED);
                SendData.engingSpeed = atoi(Rdata.can_value);
                showOnScreen(&Rdata, true, RPM);
                memset(&Rdata, 0, sizeof(Rdata));
                state = V_SPEED;
                break;
            case V_SPEED:
                strcpy(Rdata.can_pid, VEHICLE_SPEED);
                ReadData(&Rdata, V_SPEED);
                SendData.vehicleSpeed = atoi(Rdata.can_value);
                showOnScreen(&Rdata, true, KMPH);
                memset(&Rdata, 0, sizeof(Rdata));
                state = T_POSITION;
                break;
            case T_POSITION:
                strcpy(Rdata.can_pid, THROTTLE_POSITION);
                ReadData(&Rdata, T_POSITION);
                SendData.vehicleSpeed = atoi(Rdata.can_value);
                showOnScreen(&Rdata, true, ANGLE);
                memset(&Rdata, 0, sizeof(Rdata));
                state = COOLANT;
                break;
            }
        }
        xQueueSend(xDataQueue, &SendData, portMAX_DELAY);
        xTaskNotifyGive(dataprocessingHandle);
    }
}

// ====== dataProcessingTask ======
void dataProcessingTask(void *pvParameters)
{
    static ReciveData calculateReciveData[10];
    static int i = 0;

    static int time_total_value;
    static float speed_value, torque_value, bsfc_value, fcl_value, elapsed_value;
    static float pre_speed_value, engine_speed_value;
    static double fcl_total_value;

    int *time_total = &time_total_value;
    float *Torque = &torque_value;
    float *BSFC = &bsfc_value;
    float *FCL = &fcl_value;
    float *elapsed = &elapsed_value;
    float *Vehicle_speed = &speed_value;
    float *preVehicle_speed = &pre_speed_value;
    float *Engine_speed = &engine_speed_value;
    double *FCL_total = &fcl_total_value;

    /*
    static float *Torque = 0, *BSFC = 0, *FCL = 0, *elapsed = 0;
    static float *Vehicle_speed = 0, *preVehicle_speed = 0, *Engine_speed = 0;
    static double *FCL_total = 0, *time_total = 0;*/

    /*
    float *Torque = new float[2], *BSFC = new float[2], *FCL = new float[2], *elapsed = new float[2], *Vehicle_speed = new float[2], *preVehicle_speed = new float[2], *Engine_speed = new float[2];
    double *total = new double, *time = new double;
    *Torque, *BSFC = 0.0, *FCL = 0.0, *elapsed = 0.0, *Vehicle_speed = 0.0, *preVehicle_speed = 0.0, *Engine_speed = 0.0;
    *total = 0.0, *time = 0.0;*/

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ReciveData reciveDataFormTask;
        static clock_t totalTime = millis();

        clock_t startTime = 0, endTime = 0;

        if (xQueueReceive(xDataQueue, &reciveDataFormTask, portMAX_DELAY) == pdPASS)
        {
            startTime = millis();

            if (i >= 10)
            {
                i = 0;
            }
            calculateReciveData[i].num = i;
            calculateReciveData[i].coolant = reciveDataFormTask.coolant;
            calculateReciveData[i].vehicleSpeed = reciveDataFormTask.vehicleSpeed;
            calculateReciveData[i].engingSpeed = reciveDataFormTask.engingSpeed;
            calculateReciveData[i].thresholdPosition = reciveDataFormTask.thresholdPosition;

            Serial.print("Index: ");
            Serial.println(calculateReciveData[i].num);
            Serial.print("Coolant: ");
            Serial.println(calculateReciveData[i].coolant);
            Serial.print("Vehicle Speed: ");
            Serial.println(calculateReciveData[i].vehicleSpeed);
            Serial.print("Engine Speed: ");
            Serial.println(calculateReciveData[i].engingSpeed);
            Serial.print("Threshold Position: ");
            Serial.println(calculateReciveData[i].thresholdPosition);

            (*Vehicle_speed) = calculateReciveData[i].vehicleSpeed;
            Serial.print("Vehicle_speed=");
            Serial.print(*Vehicle_speed);
            Serial.println(" km/sec");

            (*Vehicle_speed) = ((*Vehicle_speed) * 5) / 18;
            Serial.print("Vehicle_speed=");
            Serial.print(*Vehicle_speed);
            Serial.println(" m/sec");

            *Engine_speed = calculateReciveData[i].engingSpeed;

            if ((*Vehicle_speed) <= 0.0)
            {
                *Engine_speed = 1000;
            }

            Serial.print("Engine_speed=");
            Serial.print(*Engine_speed);
            Serial.print(" RPM");

            *Engine_speed = (*Engine_speed) * 0.1047;

            Serial.print(" = ");
            Serial.print(*Engine_speed);
            Serial.println(" rad/sec");
            endTime = millis();
            *elapsed = (float)(endTime - startTime) / 1000; ///////////////////////////////
            *time_total += (totalTime / 1000);

            Serial.print(" elapsed Time=");
            Serial.print(*elapsed);
            Serial.println(" sec");

            *Torque = T_Engine(preVehicle_speed, Vehicle_speed, elapsed);
            (*preVehicle_speed) = (*Vehicle_speed);

            Serial.print("Torque= ");
            Serial.print(*Torque);
            Serial.println(" N-m");

            *BSFC = tablemethod(Engine_speed, Torque);
            Serial.print("BSFC= ");
            Serial.print(*BSFC);
            Serial.println(" g/kWh");

            if (isnan(*BSFC) | *BSFC <= 0)
            {
                Serial.print("    Error: BSFC is NaN or 0 at index ");
                Serial.print(i);
                Serial.println(" !\n");
                *BSFC = 250.0;
            }

            *FCL = FC_L(Engine_speed, Torque, BSFC);

            if (isnan(*FCL) | *FCL <= 0)
            {
                Serial.print("    Error: FCL is NaN or 0 at index ");
                Serial.print(i);
                Serial.println(" !\n");
                *FCL = 0.0;
            }

            Serial.print("FC(L)_value= ");
            Serial.print(*FCL, 5);
            Serial.println(" L");

            *FCL_total = *FCL_total + *FCL;
            if (isnan(*FCL_total) | *FCL_total <= 0)
            {
                Serial.print("    Error: FCL total is NaN or 0 at index ");
                Serial.print(i);
                Serial.println(" !\n");
            }

            Serial.print("FC(L)_total= ");
            Serial.print(*FCL_total, 5);
            Serial.println(" L");

            Serial.print("Test time= ");
            Serial.print(*time_total); // test time可能有錯
            Serial.println(" Sec");

            i++;

            lcd.clear();
            lcd.print("FC(L)_total=                ");
            lcd.setCursor(0, 1);
            lcd.print((*FCL_total), 8);
            lcd.print("  L");
            vTaskDelay(2500);

            lcd.clear();
            lcd.print("Test time=                ");
            lcd.setCursor(0, 1);
            lcd.print((*time_total));
            lcd.print("  Sec");
            vTaskDelay(2500);
        }
        Serial.print("Task processing over [");
        Serial.print(i);
        Serial.println("] time");
        xTaskNotifyGive(readdataHandle); // 收到Read data的資料並顯示完後，把控制權丟給Read data任務
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

    /*
    while (Serial2.available() > 0)
    {
        size_t len = Serial2.readBytesUntil('\0', Sbuffer, sizeof(Sbuffer) - 1);
        if (len > 0)
        {
            Sbuffer[len] = '\0';
            for (int i = 0; i < len; i++)
            {
                if (isprint(Sbuffer[i]))
                {
                    filteredLine[i] += Sbuffer[i];
                }
                else if (Sbuffer[i] == '\n' || Sbuffer[i] == '\r')
                {
                    filteredLine[i] += Sbuffer[i];
                }
            }

            if (sizeof(filteredLine) > 0)
            {
                Svalue = (String)filteredLine;
                for (int i = 0; i < sizeof(filteredLine); i++)
                {
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
                    Svalue.replace("\t", "");
                    Svalue.replace("⸮", "");
                    Svalue.replace("|", "");
                }
            }
        }
    }*/

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

        String pidStr = String(input->can_pid);
        Svalue.replace(pidStr, "");
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

    case NO_PID:
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

    case S_PID:
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

    case E_SPEED:
        Valuecalcu = ((256 * SvalueA) + SvalueB) / 4;
        Dvalue = (String)Valuecalcu;

        temp1 = Svalue + "  ";
        temp2 = Dvalue + "  ";

        strlcpy(input->can_title, "010C Engine Speed               ", sizeof(input->can_title) - 1);
        strlcpy(input->can_message, temp1.c_str(), sizeof(input->can_message) - 1);
        strlcpy(input->can_value, temp2.c_str(), sizeof(input->can_value) - 1);
        Serial.println("readdata case over");
        break;

    case V_SPEED:
        Valuecalcu = SvalueA;
        Dvalue = (String)Valuecalcu;

        temp1 = Svalue + "  ";
        temp2 = Dvalue + "  ";

        strlcpy(input->can_title, "010D Vehicle Speed               ", sizeof(input->can_title) - 1);
        strlcpy(input->can_message, temp1.c_str(), sizeof(input->can_message) - 1);
        strlcpy(input->can_value, temp2.c_str(), sizeof(input->can_value) - 1);
        Serial.println("readdata case over");

        break;

    case T_POSITION:
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

void showOnScreen(Vehiclemessage *input, bool state, Unit unit)
{
    if (state == true)
    {
        lcd.clear();
        lcd.print((String)input->can_title + "          ");
        lcd.setCursor(0, 1);
        lcd.print((String)input->can_message + "          ");
        vTaskDelay(2000);

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

        vTaskDelay(2000);
    }
    else
    {
        lcd.clear();
        lcd.print((String)input->can_title + "          ");
        lcd.setCursor(0, 1);
        lcd.print((String)input->can_message + "          ");
        vTaskDelay(1500);
    }
}