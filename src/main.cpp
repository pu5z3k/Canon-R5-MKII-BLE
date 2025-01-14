#include <Arduino.h>
#include "CanonBLERemote.h"

// Obiekt sterowania
CanonBLERemote canon_ble("ESP32 Canon BLE Demo");

void handleSerialCommands();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 Canon BLE Remote (zostajemy polaczeni) ===");
  Serial.println("Komendy w Serial:");
  Serial.println(" p - parowanie (15s scan)");
  Serial.println(" s - shutter (krotki impuls)");
  Serial.println(" f - focus (krotki impuls)");
  Serial.println(" h - hold shutter (5 sek)");
  Serial.println(" r - status polaczenia / zapisanego adresu");
  Serial.println(" e - erase (usun klucz NVS, zapomnij aparat)");
  Serial.println(" d - disconnect (jesli chcesz sie rozlaczyc recznie)\n");

  canon_ble.init();
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
          Serial.println("OK! (Sprawdz, czy zrobil zdjecie)");
        }
      } break;

      case 'f': {
        Serial.println("Autofocus (focus)...");
        if (!canon_ble.focus()) {
          Serial.println("Blad: focus nie powiodl sie!");
        } else {
          Serial.println("OK! (powinien zlapac ostrosc)");
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

      default:
        break;
    }
  }
}
