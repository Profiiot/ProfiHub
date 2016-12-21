//
// Created by Benjamin Skirlo on 21/12/16.
//

#ifndef FIRMWARE_SENSORDISPATCH_H
#define FIRMWARE_SENSORDISPATCH_H
#define UPP_IO_OUT "UPP-OUT"

#include "Arduino.h"
#include "Wire.h"
#include "../uppStrings.h"
#include "../communication.h"
namespace sensor_lib {

    void setup();
    void loop();

    const int ANALOG_1 = 6;
    const int ANALOG_2 = 7;

    const int DIGITAL_1  = 53;
    const int DIGITAL_2  = 51;
    UARTClass hub_Serial = Serial;
    UARTClass Serial = Serial3;

    const int DAC = DAC0;

    TwoWire Wire = Wire1;

    void send(char * _v){
        char buffer[256];
        sprintf(buffer, "%s %s", UPP_IO_OUT, _v);
        hub_Serial.println("hallo");
        tx(buffer);
    };

    void send(int * _v){
        char buffer[16];
        sprintf(buffer, "%i", _v);
        send(buffer);
    }

    void send(float _v){
        char buffer[16];
        sprintf(buffer, "%f", _v);
        send(buffer);
    }

    void send(char * _v, int length){
        char buffer[256];
        Base64.encode(buffer, _v, length);
        send(buffer);
    }

    #include "libInclude.h"
}
#endif //FIRMWARE_SENSORDISPATCH_H
