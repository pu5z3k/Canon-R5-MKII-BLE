#ifndef CANON_REMOTE_CONFIG_H_
#define CANON_REMOTE_CONFIG_H_

#include <Arduino.h>

// Struktura parametru sekwencji ostrości (np. "steps" i "totalShots")
struct FocusSequenceParam {
  int stepSize;             // ile kroków ma się przesunąć w jednym cyklu
  unsigned long totalShots; // ile razy przesunąć
};

// Główna klasa konfiguracyjna
class CanonRemoteConfig {
public:
    // -- Parametry obiektywu (zakres ostrości)
    int lensMinFocusPos = 0;
    int lensMaxFocusPos = 4000;

    // -- Opóźnienia i czasy (w ms, unless noted otherwise)
    // Czas w mikrosekundach do minimalnego spiDelay – używany w obiektywie
    int spiDelayUS = 2;
    // Opóźnienie między kolejnymi przesunięciami w stackowaniu (co "zdjęcie")
    int stepDelayMS = 20;

    // -- Parametry spustu migawki
    // Domyślnie: żadnych długich holdShutter – bo używamy pressShutter() / releaseShutter().
    // Ale możesz tu dodać np. defaultHoldTime = 30000 itp.

    // -- Tablica możliwych sekwencji (z przykładu: -10 => 74 zdjęć, -8 => 92 zdjęć, itp.)
    //  Ujemny krok = cofanie ostrości w stronę minimalnej (faktycznie my wywołamy MoveFocus(-stepSize))
    static const int SEQ_COUNT = 6; // bo mamy 6 par stepSize / totalShots
    FocusSequenceParam sequences[SEQ_COUNT] = {
        {10, 74UL},   // case '4'
        {8,  92UL},   // case '5'
        {6,  123UL},  // case '6'
        {4,  184UL},  // case '7'
        {2,  366UL},  // case '8'
        {1,  731UL}   // case '9'
    };
};

#endif
