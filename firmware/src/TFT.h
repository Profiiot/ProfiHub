//
// Created by Benjamin Skirlo on 19/12/16.
//

#ifndef FIRMWARE_TFT_H
#define FIRMWARE_TFT_H
#include <TFT.h>  // Arduino LCD library
#include <SPI.h>

// pin definition for the Uno
#define TFT_cs   10
#define TFT_dc   9
#define TFT_rst  8

// create an instance of the library
TFT TFTScreen = TFT(TFT_cs, TFT_dc, TFT_rst);

void setupTFT() {

  // Put this line at the beginning of every sketch that uses the GLCD:
  TFTScreen.begin();

  // clear the screen with a black background
  TFTScreen.background(0, 0, 0);

  // write the static text to the screen
  // set the font color to white
  TFTScreen.stroke(255, 255, 255);
}

void writeToTFT(char* text, int textSize) {
  TFTScreen.background(0, 0, 0);
  TFTScreen.setTextSize(textSize);

  // set the font color
  TFTScreen.stroke(255, 255, 255);
  // print the sensor value
  TFTScreen.text(text, 0, 20);
}
#endif //FIRMWARE_TFT_H
