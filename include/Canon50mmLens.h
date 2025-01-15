#ifndef CANON_50MM_LENS_H_
#define CANON_50MM_LENS_H_

#include <Arduino.h>
#include <SPI.h>
#include "CanonRemoteConfig.h" // Nasza klasa z parametrami

class Canon50mmLens {
public:
    Canon50mmLens(CanonRemoteConfig& cfg)
    : config(cfg) {}

    // Inicjalizacja SPI (pinów), ustawienie trybu, itd.
    void initLens();

    // Flaga do przerywania długich akcji (np. ContinuousFocusMinusX)
    bool stopOperation = false;

    // Ruch o "steps" kroków - jednorazowy
    void MoveFocus(int steps);

    // Sekwencja w pętli: minusX (wielokrotne MoveFocus(-stepSize))
    void ContinuousFocusMinusX(int stepSize, unsigned long totalShots);

    // Kalibracja obiektywu (min -> max -> min) w MAŁYCH krokach
    void calibrateLensPositions();

    // Aktualna pozycja ostrości
    int getFocuserPosition() const { return focuserPosition; }

private:
    CanonRemoteConfig& config;
    int focuserPosition = 4000;

    // Piny SPI
    static const int PIN_SPI_SCK  = 18;
    static const int PIN_SPI_MOSI = 23;
    static const int PIN_SPI_MISO = 19;
    static const int PIN_SPI_SS   = 5;

    // Metoda pomocnicza do stopniowanego ruchu (zamiast jednego wielkiego kroku)
    void moveToPositionGradually(int targetPos, int singleStepSize = 100, int stepDelayMs = 60);
};

#endif
