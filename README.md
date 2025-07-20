# Vehicle Efficiency Monitoring System

This is a simple, fundamental vehicle status monitoring system that captures vehicle data and calculates the current fuel consumption rate.

## Table of Contents

- [About the Project](#about-the-project)
- [Key Features](#key-features)
- [Installation](#installation)
- [Usage](#usage)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)
- [Contact/Acknowledgments](#contactacknowledgments)

## Hardware Configuration
  * main microcontroller:NodeMCU-32S(you can choose any what you like).
  * 12V to 5V convert module.
  * Vehicle BUS module(i'm use ELM327, but)
  * Display module(i'm use 16*2 LCD)
//               |----->|12VIN  12V convert to         |   DC 5V    
//               |      |          5V module      5VOUT|---------->|VIN         N            |
//           DC  |                                                 |            o            |   
//           12V |                                                 |            d            |        >-------->|VCC                    |
//               |                                                 |            e         SCL|----------------->|SCL  16X2 LCD display  |
//               |                                                 |            M         SDA|----------------->|SDA  I2C protocol      |
//OBD II     |   |   |ELM327      UART|                            |            C            |       >-------->||Gnd                    |
//Diagnostic |<----->|Interpreter   TX|<-------------------------->|Serial1.RX  U            |
//Interface  |       |Module        RX|<-------------------------->|Serial1.TX  -            |
//                                                                 |            3            |
//                                                                 |            2            |
//                                                                 |            S            |
    


## Key Features

- Feature 1
- Feature 2
- Feature 3

## Installation

Step-by-step installation instructions.

