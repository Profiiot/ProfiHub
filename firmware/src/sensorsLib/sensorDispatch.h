//
// Created by Benjamin Skirlo on 21/12/16.
//

#ifndef FIRMWARE_SENSORDISPATCH_H
#define FIRMWARE_SENSORDISPATCH_H
#include "Arduino.h"
#include "Wire.h"
namespace sensor_lib {

    void setup();
    void loop();

    const int ANALOG_1 = 6;
    const int ANALOG_2 = 7;

    const int DIGITAL_1 = 53;
    const int DIGITAL_2 = 51;
    UARTClass Serial = Serial3;

    const int DAC = DAC0;

    TwoWire Wire = Wire1;



    #include "libInclude.h"
}
#endif //FIRMWARE_SENSORDISPATCH_H
