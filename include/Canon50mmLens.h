#ifndef CANON_50MM_LENS_H_
#define CANON_50MM_LENS_H_

#include <Arduino.h>
#include <SPI.h>
#include "CanonRemoteConfig.h"

/**
 * Klasa do obsługi obiektywu przez SPI.
 *
 * Zakładamy: 4000 = start (wsunięty), 0 = stop (wysunięty).
 * 
 * Metody kluczowe:
 *  - initLens(): pełny przejazd 4000->0->4000 + ustawienie pozycji = 4000
 *  - initLensReturnToStart(): jedzie do 4000
 *  - initLensTest(): (opcjonalny test)
 *  - MoveFocusSmooth(...): płynny ruch jednorazowy
 *  - moveToPositionGradually(...): jeśli potrzebny krokowy
 *  - ContinuousFocusStartToStop(...): sekwencje 4..9, jedzie 4000->0 w krokach
 */
class Canon50mmLens {
public:
    Canon50mmLens(CanonRemoteConfig& cfg) : config(cfg) {}

    // Inicjalizacja SPI + pełny przejazd w obie strony
    void initLens();

    // Flaga do przerywania ruchu
    bool stopOperation = false;

    // Ruch testowy (jeśli potrzebny): 4000->2000->4000
    void initLensTest();

    // Jedzie do 4000
    void initLensReturnToStart();

    // Ruch płynny jednorazowy
    void MoveFocusSmooth(int steps);

    // Ruch krokowy (jeśli potrzebne)
    void moveToPositionGradually(int targetPos, int singleStepSize=100, int stepDelayMs=60);

    // Sekwencja: jedzie z 4000->0, pauza, powrót do 4000
    void ContinuousFocusStartToStop(int x, unsigned long ileZdjec);

    // Aktualna pozycja
    int getFocuserPosition() const { return focuserPosition; }

private:
    CanonRemoteConfig& config;
    // Bieżąca pozycja (logiczna)
    int focuserPosition = 4000; 

    // Piny SPI
    static const int PIN_SPI_SCK  = 18;
    static const int PIN_SPI_MOSI = 23;
    static const int PIN_SPI_MISO = 19;
    static const int PIN_SPI_SS   = 5;

    // rangeMin=0, rangeMax=4000
    inline int rangeMin() {
        return (config.lensMinFocusPos < config.lensMaxFocusPos) 
            ? config.lensMinFocusPos : config.lensMaxFocusPos;
    }
    inline int rangeMax() {
        return (config.lensMinFocusPos > config.lensMaxFocusPos) 
            ? config.lensMinFocusPos : config.lensMaxFocusPos;
    }
};

#endif
