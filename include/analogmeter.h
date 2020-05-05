#include "Arduino.h"
#include "TFT_eSPI.h"
#include <SPI.h>
#define TFT_FRAME       0x5AEB
#define TFT_BACKGROUND  0xaaaa
#define M_SIZE 1.0
#ifndef _ANALOGMETER_H
#define _ANALOGMETER_H

class Analogmeter
{
    private:
    TFT_eSPI& tft;
    int old_analog;
    uint16_t osx, osy;
    float ltx;

    public:
    Analogmeter(TFT_eSPI& tft_):tft(tft_)
    {
        old_analog  = -999; // Value last displayed
        osx         = M_SIZE*120; 
        osy         = M_SIZE*120; // Saved x & y coords
        ltx         = 0;    // Saved x coord of bottom of needle
    }
    void plot(void);
    void plotNeedle(int value, byte ms_delay);
};
#endif