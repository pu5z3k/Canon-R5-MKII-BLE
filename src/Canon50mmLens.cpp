#include "Canon50mmLens.h"

void Canon50mmLens::initLens() {
    stopOperation = false;
    focuserPosition = config.lensMaxFocusPos; // Domyślnie na max (np. 4000)

    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SS);
    SPI.setFrequency(1000000);    // 1 MHz
    SPI.setDataMode(SPI_MODE0);   // standard
    SPI.setBitOrder(MSBFIRST);

    pinMode(PIN_SPI_SS, OUTPUT);
    digitalWrite(PIN_SPI_SS, HIGH);
}

void Canon50mmLens::MoveFocus(int steps) {
    int newPos = focuserPosition + steps;
    if (newPos < config.lensMinFocusPos) newPos = config.lensMinFocusPos;
    if (newPos > config.lensMaxFocusPos) newPos = config.lensMaxFocusPos;

    int realSteps = newPos - focuserPosition;
    if (realSteps == 0) return;

    // Wyliczamy bajty do przesłania
    uint8_t x = highByte(realSteps);
    uint8_t y = lowByte(realSteps);

    // SS w dół
    digitalWrite(PIN_SPI_SS, LOW);
    delayMicroseconds(config.spiDelayUS);

    SPI.transfer(0x0A); delayMicroseconds(config.spiDelayUS);
    SPI.transfer(0x44); delayMicroseconds(config.spiDelayUS);
    SPI.transfer(x);    delayMicroseconds(config.spiDelayUS);
    SPI.transfer(y);    delayMicroseconds(config.spiDelayUS);
    SPI.transfer(0x00); delayMicroseconds(config.spiDelayUS);

    digitalWrite(PIN_SPI_SS, HIGH);
    delayMicroseconds(config.spiDelayUS);

    focuserPosition = newPos;
}

void Canon50mmLens::ContinuousFocusMinusX(int stepSize, unsigned long totalShots) {
    stopOperation = false; // start sekwencji
    for (unsigned long i = 0; i < totalShots; i++) {
        if (stopOperation) {
            // przerwanie
            break;
        }
        MoveFocus(-stepSize);
        // Opóźnienie między krokami
        delay(config.stepDelayMS);
    }
}
