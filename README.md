# Vehicle Efficiency Monitoring System

This is a simple, fundamental vehicle status monitoring system that captures vehicle data and calculates the current fuel consumption rate.

## Table of Contents

- [Hardware Configuration](#hardware-configuration)
- [Operation](#operation)



## Key Features

- Feature 1
- Feature 2
- Feature 3
- 

## Operation
Given the driving distance and assuming the test road is flat with no climbing, define the discrete vehicle speed as $$v$$  , and the driving time as $$t$$  .   
Therefore, we can obtain the average acceleration as $$a_{vehicle}$$ using the discrete formula:  

$$
a = \frac{v_i - v_{i-1}}{t_i - t_{i-1}}
$$  

To calculate the tire driving force, according to the principles of dynamics, the required tire driving force is:

$$
F_tire = m_{vehicle}a_{vehicle}+F_{rolling}+F_{air}
$$  

Where the mass of the vehicle is denoted as **$$m_{vehicle}$$**  , the rolling resistance force as **$$F_{rolling}$$**  , and the aerodynamic force as **$$F_{air}$$**  .



## Hardware Configuration
  * main microcontroller:NodeMCU-32S(you can choose any what you like).
  * 12V to 5V convert module.
  * Vehicle BUS module(This repository uses an ELM327 module because our research target is a scooter, so it needs to be flexible.)
  * Display module(This repository uses a 16x2 LCD.)

<pre>
               |----->|IN  12V convert to    |   DC 5V   |                      |
               |      |      5V module    OUT|---------->|VIN         N         |
           DC  |                                         |            o         |   
           12V |                                         |            d         |    >-------->|VCC                  |
               |                                         |            e      SCL|------------->|SCL  16X2 LCD display|
               |                                         |            M      SDA|------------->|SDA  I2C protocol    |
OBD II     |   |   |ELM327      UART|                    |            C         |    >-------->|Gnd                  |
Diagnostic |<----->|Interpreter   TX|<------------------>|Serial1.RX  U         |
Interface  |       |Module        RX|<------------------>|Serial1.TX  -         |
                                                         |            3         |
                                                         |            2         |
                                                         |            S         |
</pre>                                                   




## Installation

Step-by-step installation instructions.

