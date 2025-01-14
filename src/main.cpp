#include <Arduino.h>
#include "CanonBLERemote.h"
#include "Canon50mmLens.h"
#include "CanonRemoteConfig.h"

// GLOBALNE OBIEKTY
CanonRemoteConfig  globalConfig;            // parametry w jednym miejscu
CanonBLERemote     canon_ble("ESP32 Canon BLE Demo");
Canon50mmLens      lens(globalConfig);      // Konstruktor z referencją do globalConfig

// Funkcja do obsługi sekwencji: wciśnij migawkę → MoveFocus → zwolnij
void executeFocusSequence(int seqIndex);

// Obsługa serial
void handleSerialCommands();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 Canon BLE Remote + Canon50mm Lens (v1.1) ===");
  Serial.println("Komendy w Serial:");
  Serial.println(" p - parowanie (15s scan) [BLE]");
  Serial.println(" s - shutter (krotki impuls) [BLE]");
  Serial.println(" f - focus (krotki impuls)   [BLE]");
  Serial.println(" h - hold shutter (5 sek)    [BLE]");
  Serial.println(" r - status polaczenia / zapisanego adresu [BLE]");
  Serial.println(" e - erase (usun klucz NVS)  [BLE]");
  Serial.println(" d - disconnect [BLE]");
  Serial.println(" o - init lens (SPI)");
  Serial.println(" k - przerwij cykl (stopOperation=true) + zwolnij migawke");
  Serial.println(" 4..9 - wywołanie sekwencji (z globalConfig.sequences)");
  Serial.println();

  canon_ble.init();   // Inicjalizacja BLE
  lens.initLens();    // Inicjalizacja obiektywu
}

void loop() {
  handleSerialCommands();
  delay(20);
}

void handleSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'p': {
        Serial.println("Parowanie... (wlacz Remote Pairing w Canonie, 15s scan)");
        if (canon_ble.pair(15)) {
          Serial.println("Sparowano pomyslnie!");
        } else {
          Serial.println("Parowanie nie powiodlo sie.");
        }
      } break;

      case 's': {
        Serial.println("Wyzwolenie migawki (trigger)...");
        if (!canon_ble.trigger()) {
          Serial.println("Blad: trigger nie powiodl sie!");
        } else {
          Serial.println("OK!");
        }
      } break;

      case 'f': {
        Serial.println("Autofocus (focus)...");
        if (!canon_ble.focus()) {
          Serial.println("Blad: focus nie powiodl sie!");
        } else {
          Serial.println("OK!");
        }
      } break;

      case 'h': {
        Serial.println("Hold shutter przez 5 sek...");
        if (!canon_ble.holdShutter(5000)) {
          Serial.println("Blad: holdShutter nie powiodl sie!");
        } else {
          Serial.println("OK! Zwolnil spust po 5s");
        }
      } break;

      case 'r': {
        bool connected = canon_ble.isConnected();
        String addr = canon_ble.getPairedAddressString();
        Serial.printf("Polaczenie: %s\n", connected ? "TAK" : "NIE");
        Serial.printf("Zapamietany adres: %s\n", addr.c_str());
      } break;

      case 'e': {
        Serial.println("Usuwanie klucza 'cameraaddr' w NVS...");
        canon_ble.eraseSavedAddress();
        Serial.println("OK. Mozesz ponownie sparowac ('p').");
      } break;

      case 'd': {
        Serial.println("Rozlaczam...");
        canon_ble.disconnect();
      } break;

      case 'o': {
        Serial.println("Inicjalizacja obiektywu (SPI)...");
        lens.initLens();
        Serial.println("OK. lens.focuserPosition = 4000.");
      } break;

      case 'k': {
        // Przerwanie cyklu
        Serial.println("Ustawiam stopOperation = true i zwalniam migawke (if pressed)...");
        lens.stopOperation = true;
        // Zwolnienie spustu
        canon_ble.releaseShutter();
      } break;

      // Sekwencje "4..9" - mapujemy na indexy 0..5
      case '4': executeFocusSequence(0); break;
      case '5': executeFocusSequence(1); break;
      case '6': executeFocusSequence(2); break;
      case '7': executeFocusSequence(3); break;
      case '8': executeFocusSequence(4); break;
      case '9': executeFocusSequence(5); break;

      default:
        // Nieznany znak
        break;
    }
  }
}

/**
 * Funkcja, która:
 *  1) Wciska migawkę (pressShutter())
 *  2) Wykonuje sekwencję ContinuousFocusMinusX(...) (wg globalConfig.sequences[seqIndex])
 *  3) Po zakończeniu (lub przerwaniu) zwalnia migawkę (releaseShutter()).
 */
void executeFocusSequence(int seqIndex) {
  if (seqIndex < 0 || seqIndex >= globalConfig.SEQ_COUNT) {
    Serial.println("Zly indeks sekwencji!");
    return;
  }

  // Pobierz parametry z globalConfig
  FocusSequenceParam param = globalConfig.sequences[seqIndex];

  // Wciskamy spust
  bool okPress = canon_ble.pressShutter();
  if (!okPress) {
    Serial.println("Blad: nie udalo sie wcisnac migawki!");
    return;
  }

  lens.stopOperation = false;
  Serial.printf("Rozpoczynam sekwencje: stepSize=%d, totalShots=%lu\n",
                 param.stepSize, param.totalShots);

  // Wykonaj przesuwanie ostrości
  lens.ContinuousFocusMinusX(param.stepSize, param.totalShots);

  // Po zakończeniu (lub przerwaniu 'k') - zwalniamy spust
  Serial.println("Koniec sekwencji / przerwanie -> release shutter...");
  canon_ble.releaseShutter();
}
