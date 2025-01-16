#include "Canon50mmLens.h"

static const int spiDelay = 2;  
static const int stepDelay = 20; 

void Canon50mmLens::initLens() {
    stopOperation = false;

    // Inicjalizacja SPI
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SS);
    SPI.setFrequency(1000000);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE3);
    pinMode(PIN_SPI_SS, OUTPUT);
    digitalWrite(PIN_SPI_SS, HIGH);

    // Pełny przejazd 4000->0->4000, wolniejszy (np. singleStepSize=100, stepDelayMs=50)
    focuserPosition = 4000;
    Serial.println("[Lens] initLens: rozpoczynam pełny przejazd 4000->0->4000 (kalibracja).");
    moveToPositionGradually(config.lensMaxFocusPos, 100, 50); // 4000->0
    if (!stopOperation) {
      moveToPositionGradually(config.lensMinFocusPos, 100, 50); // 0->4000
    }

    Serial.println("[Lens] Kalibracja zakończona, obiektyw w pozycji 4000 (start).");
}

/**
 * Test: 4000->2000->4000
 */
void Canon50mmLens::initLensTest() {
    stopOperation = false;
    Serial.println("[Lens] initLensTest: 4000->2000->4000");

    int midPos = 2000;

    if (focuserPosition > midPos) {
        moveToPositionGradually(midPos, 100, 30);
    }
    if (!stopOperation) {
      moveToPositionGradually(4000, 100, 30);
    }

    Serial.printf("[Lens] test done. obiektyw w pozycji = %d\n", focuserPosition);
}

/**
 * Wraca do start=4000
 */
void Canon50mmLens::initLensReturnToStart() {
    stopOperation = false;
    Serial.println("[Lens] initLensReturnToStart: jedziemy do 4000");
    moveToPositionGradually(4000, 100, 40);
    Serial.printf("[Lens] Gotowe, stoimy w %d\n", focuserPosition);
}

/**
 * MoveFocusSmooth(steps)
 *  - 1-razowy płynny ruch
 */
void Canon50mmLens::MoveFocusSmooth(int steps) {
    int newPos = focuserPosition + steps;

    // Zawężenie do [0..4000]
    if(newPos < rangeMin()) newPos = rangeMin();
    if(newPos > rangeMax()) newPos = rangeMax();

    int realSteps = newPos - focuserPosition;
    if(realSteps == 0) return;

    // Odwracanie kierunku (jeśli flaga w config jest ustawiona)
    if (config.invertFocusDirection) {
        realSteps = -realSteps;
    }

    digitalWrite(PIN_SPI_SS, LOW);

    uint8_t x = highByte(realSteps);
    uint8_t y = lowByte(realSteps);

    SPI.transfer(0x0A); delay(spiDelay);
    SPI.transfer(0x44); delay(spiDelay);
    SPI.transfer(x);    delay(spiDelay);
    SPI.transfer(y);    delay(spiDelay);
    SPI.transfer(0x00); delay(spiDelay);

    delay(2);

    digitalWrite(PIN_SPI_SS, HIGH);

    focuserPosition = newPos;
}

/**
 * moveToPositionGradually():
 *  - pętla, w której wywołujemy MoveFocusSmooth małymi paczkami
 */
void Canon50mmLens::moveToPositionGradually(int targetPos, int singleStepSize, int stepDelayMs) {
    while (!stopOperation && (focuserPosition != targetPos)) {
        int diff = targetPos - focuserPosition;
        int sign = (diff > 0) ? 1 : -1;
        int stepNow = abs(diff);
        if (stepNow > singleStepSize) stepNow = singleStepSize;

        MoveFocusSmooth(sign * stepNow);
        delay(stepDelayMs);
    }
}

/**
 * Sekwencja 4..9 -> 4000->0
 * Po zakończeniu 1 sek pauzy i powrót do 4000
 */
void Canon50mmLens::ContinuousFocusStartToStop(int x, unsigned long ileZdjec) {
    stopOperation = false;

    // Upewniamy się, że stoimy w 4000
    if (focuserPosition != 4000 && !stopOperation) {
        moveToPositionGradually(4000, 100, 40);
    }
    if (stopOperation) {
      Serial.println("[Lens] PRZERWANO przed sekwencją, zostajemy w biezacej pozycji");
      return;
    }

    Serial.printf("[Lens] Sekwencja: 4000->0, step=%d, zdj=%lu.\n", x, ileZdjec);
    Serial.println("[Lens] MIGAWKA ON (symbolicznie)");

    // Jedziemy w dół do 0
    while (!stopOperation && focuserPosition > 0) {
        MoveFocusSmooth(-x);
        delay(10);
    }

    Serial.println("[Lens] MIGAWKA OFF");

    if (stopOperation) {
        Serial.println("[Lens] PRZERWANO w trakcie sekwencji");
        return;
    }

    // finalnie stoimy w 0
    Serial.println("[Lens] Osiągnięto 0 (stop). 1s pauzy...");
    delay(1000);

    // powrót do 4000
    Serial.println("[Lens] Powrót do 4000");
    moveToPositionGradually(4000, 100, 40);

    Serial.println("[Lens] Koniec sekwencji, wróciliśmy do 4000 (start).");
}
