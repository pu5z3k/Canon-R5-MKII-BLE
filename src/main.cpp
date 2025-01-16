#include <Arduino.h>
#include "CanonBLERemote.h"
#include "Canon50mmLens.h"
#include "CanonRemoteConfig.h"

// Globalne obiekty
CanonRemoteConfig  globalConfig;            
CanonBLERemote     canon_ble("ESP32 Canon BLE Demo");
Canon50mmLens      lens(globalConfig);

void handleSerialCommands();
void runFocusSequenceStartToStop(int seqIndex); // 4000->0

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Jeżeli okaże się, że ruch nadal jest odwrotny -> odkomentuj:
  // globalConfig.invertFocusDirection = true;

  Serial.println("\n=== ESP32 Canon BLE Remote v2.0 (start=4000 [wsunięty], stop=0 [wysunięty]) ===");
  Serial.println("Komendy:");
  Serial.println(" p - parowanie(15s)");
  Serial.println(" s - shutter (trigger) | m - shutter (trigger) ");
  Serial.println(" f - focus");
  Serial.println(" h - hold(5s)");
  Serial.println(" r - BLE status/addr");
  Serial.println(" e - eraseNVS");
  Serial.println(" d - disconnect");
  Serial.println(" k - przerwanie (stopOperation, powtórna inicjalizacja obiektywu)");
  Serial.println(" o - initLensReturnToStart (do 4000)");
  Serial.println(" t - initLensTest (4000->2000->4000)");
  Serial.println(" 4..9 - sekwencja start->stop (4000->0), potem powrót do 4000");
  Serial.println();

  // BLE
  canon_ble.init();

  // Obiektyw - automatyczna kalibracja (full pass)
  lens.initLens();
}

void loop() {
  handleSerialCommands();
  delay(20);
}

void handleSerialCommands() {
  if (!Serial.available()) return;
  char cmd = Serial.read();

  switch (cmd) {
    case 'p':
      Serial.println("Parowanie (15s)...");
      if (canon_ble.pair(15)) {
        Serial.println("Sparowano pomyślnie!");
      } else {
        Serial.println("Parowanie nie powiodło się.");
      }
      break;

    case 's':
    case 'm':  // nowa funkcja "m" - również wyzwolenie migawki
      Serial.println("Shutter (trigger)...");
      if (!canon_ble.trigger()) {
        Serial.println("Błąd: trigger!");
      } else {
        Serial.println("OK!");
      }
      break;

    case 'f':
      Serial.println("Focus...");
      if (!canon_ble.focus()) {
        Serial.println("Błąd: focus!");
      } else {
        Serial.println("OK!");
      }
      break;

    case 'h':
      Serial.println("holdShutter(5s)...");
      if (!canon_ble.holdShutter(5000)) {
        Serial.println("Błąd: holdShutter!");
      } else {
        Serial.println("OK!");
      }
      break;

    case 'r': {
      bool connected = canon_ble.isConnected();
      String addr = canon_ble.getPairedAddressString();
      Serial.printf("Połączenie: %s\n", connected ? "TAK" : "NIE");
      Serial.printf("Aparat addr: %s\n", addr.c_str());
    } break;

    case 'e':
      Serial.println("eraseSavedAddress()...");
      canon_ble.eraseSavedAddress();
      Serial.println("OK. Możesz ponownie sparować (p).");
      break;

    case 'd':
      Serial.println("disconnect()...");
      canon_ble.disconnect();
      break;

    case 'k':
      Serial.println("PRZERWANIE: stopOperation = true, re-inicjalizacja obiektywu...");
      lens.stopOperation = true;
      // Jeśli migawka jest wciśnięta - zwolnij (opcjonalnie):
      canon_ble.releaseShutter();
      // Ponowna pełna inicjalizacja (pełny przejazd)
      lens.initLens();
      break;

    case 'o':
      Serial.println("initLensReturnToStart => jedziemy do 4000");
      lens.stopOperation = false;
      lens.initLensReturnToStart();
      break;

    case 't':
      Serial.println("initLensTest => 4000->2000->4000");
      lens.stopOperation = false;
      lens.initLensTest();
      break;

    // sekwencje 4..9 => 4000->0 i powrót do 4000
    case '4': runFocusSequenceStartToStop(0); break;
    case '5': runFocusSequenceStartToStop(1); break;
    case '6': runFocusSequenceStartToStop(2); break;
    case '7': runFocusSequenceStartToStop(3); break;
    case '8': runFocusSequenceStartToStop(4); break;
    case '9': runFocusSequenceStartToStop(5); break;

    default:
      Serial.println("Nieznane polecenie.");
      break;
  }
}

void runFocusSequenceStartToStop(int seqIndex) {
  if (seqIndex < 0 || seqIndex >= globalConfig.SEQ_COUNT) {
    Serial.println("Błędny indeks sekwencji!");
    return;
  }
  FocusSequenceParam p = globalConfig.sequences[seqIndex];
  lens.stopOperation = false;
  lens.ContinuousFocusStartToStop(p.stepSize, p.totalShots);
}
