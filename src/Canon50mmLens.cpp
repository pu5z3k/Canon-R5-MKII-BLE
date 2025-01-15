#include "Canon50mmLens.h"

void Canon50mmLens::initLens() {
    stopOperation = false;
    focuserPosition = config.lensMaxFocusPos; // domyślnie 4000

    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SS);

    // Według Twojego sprawdzonego kodu
    SPI.setFrequency(1000000); // 1 MHz
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE3);

    pinMode(PIN_SPI_SS, OUTPUT);
    digitalWrite(PIN_SPI_SS, HIGH);
}

/**
 * MoveFocus(steps) – ruch jednorazowy (zakres, SPI transfer).
 */
void Canon50mmLens::MoveFocus(int steps) {
    int newPos = focuserPosition + steps;
    if (newPos < config.lensMinFocusPos || newPos > config.lensMaxFocusPos) {
        Serial.println("[Lens] Próba wyjścia poza zakres autofokusa!");
        return; 
    }

    uint8_t x = highByte(steps);
    uint8_t y = lowByte(steps);

    digitalWrite(PIN_SPI_SS, LOW);

    SPI.transfer(0x0A);
    delay(15);

    SPI.transfer(0x44);
    delay(5);

    SPI.transfer(x);
    delay(5);

    SPI.transfer(y);
    delay(5);

    SPI.transfer(0x00);
    delay(10);

    digitalWrite(PIN_SPI_SS, HIGH);

    focuserPosition = newPos;
}

/**
 * ContinuousFocusMinusX(stepSize, totalShots)
 * – wielokrotne MoveFocus(-stepSize) z przerwą config.stepDelayMS
 */
void Canon50mmLens::ContinuousFocusMinusX(int stepSize, unsigned long totalShots) {
    stopOperation = false;
    for (unsigned long i = 0; i < totalShots; i++) {
        if (stopOperation) {
            Serial.println("[Lens] Przerwano ruch obiektywu (stopOperation=true).");
            break;
        }
        MoveFocus(-stepSize);
        delay(config.stepDelayMS);
    }
}

/**
 * Funkcja pomocnicza: jedź w małych krokach do 'targetPos'
 * np. stepSize=100, stepDelayMs=60.
 */
void Canon50mmLens::moveToPositionGradually(int targetPos, int singleStepSize, int stepDelayMs) {
    // Pętla, dopóki nie osiągniemy targetPos
    // i dopóki stopOperation nie jest true
    while (focuserPosition != targetPos) {
        if (stopOperation) {
            Serial.println("[Lens] Przerwano moveToPositionGradually (stopOperation=true).");
            return;
        }
        int remaining = targetPos - focuserPosition;
        int sign = (remaining > 0) ? 1 : -1;

        // Jeśli mały "krok" przekracza to, co zostało, zmniejszamy go
        int stepNow = abs(remaining);
        if (stepNow > singleStepSize) stepNow = singleStepSize;

        MoveFocus(sign * stepNow);
        delay(stepDelayMs);
    }
}

/**
 * Kalibracja obiektywu: min -> max -> min
 * Każdy odcinek robimy w pętli małych kroków
 */
void Canon50mmLens::calibrateLensPositions() {
    Serial.println("[Lens] Kalibracja: MIN -> MAX -> MIN");

    // 1) zjedź do minFocus
    if (focuserPosition != config.lensMinFocusPos) {
        moveToPositionGradually(config.lensMinFocusPos, 100, 60);
    }
    Serial.println("  => obiektyw w pozycji minimalnej.");

    // 2) jedź do maxFocus
    if (focuserPosition != config.lensMaxFocusPos) {
        moveToPositionGradually(config.lensMaxFocusPos, 100, 60);
    }
    Serial.println("  => obiektyw w pozycji maksymalnej.");

    // 3) wróć do minFocus
    if (focuserPosition != config.lensMinFocusPos) {
        moveToPositionGradually(config.lensMinFocusPos, 100, 60);
    }
    Serial.println("  => obiektyw ponownie w minFocus. Kalibracja zakończona.");
}
