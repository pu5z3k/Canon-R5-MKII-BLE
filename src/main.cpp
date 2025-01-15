#include <Arduino.h>
#include "CanonBLERemote.h"
#include "Canon50mmLens.h"
#include "CanonRemoteConfig.h"

// GLOBALNE OBIEKTY
CanonRemoteConfig  globalConfig;            // Parametry w jednym miejscu
CanonBLERemote     canon_ble("ESP32 Canon BLE Demo");
Canon50mmLens      lens(globalConfig);      // Konstruktor z referencją do globalConfig

void executeFocusSequence(int seqIndex);
void handleSerialCommands();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 Canon BLE Remote + Canon50mm Lens (v1.1) ===");
  Serial.println("Komendy w Serial:");
  Serial.println(" p - parowanie (15s scan) [BLE]");
  Serial.println(" s - shutter (krótki impuls) [BLE]");
  Serial.println(" f - focus (krótki impuls)   [BLE]");
  Serial.println(" h - hold shutter (5 sek)    [BLE]");
  Serial.println(" r - status połączenia / zapisanego adresu [BLE]");
  Serial.println(" e - erase (usuń klucz NVS)  [BLE]");
  Serial.println(" d - disconnect [BLE]");
  Serial.println(" o - init lens (SPI) -> kalibracja min->max->min");
  Serial.println(" k - przerwij cykl (stopOperation=true) + zwolnij migawkę");
  Serial.println(" 4..9 - wywołanie sekwencji (z globalConfig.sequences)");
  Serial.println();

  canon_ble.init();      // Inicjalizacja BLE
  lens.initLens();       // Inicjalizacja obiektywu (SPI)
  lens.calibrateLensPositions(); // Od razu kalibracja min->max->min
}

void loop() {
  handleSerialCommands();
  delay(20);
}

void handleSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {

      case 'p':
        Serial.println("Parowanie... (włącz Remote Pairing w Canonie, 15s scan)");
        if (canon_ble.pair(15)) {
          Serial.println("Sparowano pomyślnie!");
        } else {
          Serial.println("Parowanie nie powiodło się.");
        }
        break;

      case 'o':
        Serial.println("=> Ponowna kalibracja obiektywu (min->max->min) w małych krokach...");
        lens.stopOperation = false;
        lens.calibrateLensPositions();
        break;

      case 's':
        Serial.println("Wyzwolenie migawki (trigger)...");
        if (!canon_ble.trigger()) {
          Serial.println("Błąd: trigger nie powiodł się!");
        } else {
          Serial.println("OK!");
        }
        break;

      case 'f':
        Serial.println("Autofocus (focus)...");
        if (!canon_ble.focus()) {
          Serial.println("Błąd: focus nie powiodł się!");
        } else {
          Serial.println("OK!");
        }
        break;

      case 'h':
        Serial.println("Hold shutter przez 5 sek...");
        if (!canon_ble.holdShutter(5000)) {
          Serial.println("Błąd: holdShutter nie powiodł się!");
        } else {
          Serial.println("OK! Zwolnił spust po 5s");
        }
        break;

      case 'r':
      {
        bool connected = canon_ble.isConnected();
        String addr = canon_ble.getPairedAddressString();
        Serial.printf("Połączenie: %s\n", connected ? "TAK" : "NIE");
        Serial.printf("Zapamiętany adres: %s\n", addr.c_str());
      }
      break;

      case 'e':
        Serial.println("Usuwanie klucza 'cameraaddr' w NVS...");
        canon_ble.eraseSavedAddress();
        Serial.println("OK. Możesz ponownie sparować ('p').");
        break;

      case 'd':
        Serial.println("Rozłączam...");
        canon_ble.disconnect();
        break;

      case 'k':
        Serial.println("Ustawiam stopOperation = true i zwalniam migawkę (if pressed)...");
        lens.stopOperation = true;
        canon_ble.releaseShutter();
        break;

      case '4': case '5': case '6': case '7': case '8': case '9':
        executeFocusSequence(cmd - '4');  // ASCII -> indeks
        break;

      default:
        Serial.println("Nieznane polecenie.");
        break;
    }
  }
}

void executeFocusSequence(int seqIndex) {
  if (seqIndex < 0 || seqIndex >= globalConfig.SEQ_COUNT) {
    Serial.println("Zły indeks sekwencji!");
    return;
  }

  FocusSequenceParam param = globalConfig.sequences[seqIndex];

  if (!canon_ble.pressShutter()) {
    Serial.println("Błąd: nie udało się wcisnąć migawki!");
    return;
  }

  lens.stopOperation = false;
  Serial.printf("Rozpoczynam sekwencję: stepSize=%d, totalShots=%lu\n",
                 param.stepSize, param.totalShots);

  // Ruch minusX w pętli
  lens.ContinuousFocusMinusX(param.stepSize, param.totalShots);

  Serial.println("Koniec sekwencji / przerwanie -> release shutter...");
  canon_ble.releaseShutter();
}
