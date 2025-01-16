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
    // Główne wartości: 4000 = start (wsunięty), 0 = stop (wysunięty)
    int lensMinFocusPos = 4000;  // start
    int lensMaxFocusPos = 0;     // stop

    // Flaga do odwrócenia kierunku (jeśli by się okazało, że jeszcze ruch jest odwrotny)
    bool invertFocusDirection = false;

    // Podstawowe parametry
    int spiDelayUS = 2;
    int stepDelayMS = 20;

    // Dla sekwencji 4..9 -> jedziemy 4000->0
    static const int SEQ_COUNT = 6;
    FocusSequenceParam sequences[SEQ_COUNT] = {
        {10, 74UL},
        {8,  92UL},
        {6,  123UL},
        {4,  184UL},
        {2,  366UL},
        {1,  731UL}
    };
};

#endif
