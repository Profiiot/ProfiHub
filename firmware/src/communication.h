//
// Created by Benjamin Skirlo on 21/12/16.
//

#ifndef FIRMWARE_COMMUNICATION_H
#define FIRMWARE_COMMUNICATION_H
#include <Arduino.h>

void tx(String message, bool awaiting_acknowledgement = false){
    Serial.print(message);
    Serial.println();
}

void tx(char * message, char * more){
    char buffer[256];
    sprintf(buffer, "%s %s", message, more);
    tx(buffer);
}
#endif //FIRMWARE_COMMUNICATION_H
