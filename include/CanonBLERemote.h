#ifndef CANON_BLE_REMOTE_H_
#define CANON_BLE_REMOTE_H_

#include <BLEDevice.h>
#include <Arduino.h>
#include <ArduinoNvs.h>

class advdCallback : public BLEAdvertisedDeviceCallbacks {
private:
    BLEAddress *paddress_to_connect;
    BLEUUID service_uuid_wanted;
    bool *pready_to_connect;

public:
    advdCallback(BLEUUID service_uuid, bool *ready_to_connect, BLEAddress *address_to_connect);
    void onResult(BLEAdvertisedDevice advertisedDevice);
};

class ConnectivityState : public BLEClientCallbacks {
private:
    bool connected = false;

public:
    void onConnect(BLEClient *pclient) override;
    void onDisconnect(BLEClient *pclient) override;
    bool isConnected();
};

class CanonBLERemote {
public:
    CanonBLERemote(String name);

    /**
     * Inicjalizacja BLE, wczytanie parowania z NVS.
     */
    void init();

    /**
     * Skan + parowanie z aparatem (z trybem BLE Remote).
     * Domyślnie 15s (ale można podać dowolne).
     */
    bool pair(unsigned int scan_duration = 15);

    /**
     * Czy obecnie mamy aktywne połączenie BLE z aparatem?
     */
    bool isConnected();

    /**
     * Wyzwolenie migawki (krótki impuls).
     * UWAGA: w tym kodzie NIE rozłączamy się od razu,
     *        dzięki czemu 'r' pokaże "TAK".
     */
    bool trigger();

    /**
     * Autofocus (krótki impuls).
     * Podobnie – nie rozłączamy się domyślnie.
     */
    bool focus();

    /**
     * Przytrzymanie migawki przez holdTimeMs, potem zwolnienie.
     * Tutaj na końcu też nie rozłączamy się (możesz ewentualnie odkomentować).
     */
    bool holdShutter(unsigned long holdTimeMs);

    /**
     * Odczytanie adresu zapisanego w NVS (lub 00:00:00... jeśli brak).
     */
    String getPairedAddressString();

    /**
     * Skasowanie z NVS zapisanego klucza "cameraaddr".
     * (np. gdy chcesz wymusić ponowne parowanie).
     */
    void eraseSavedAddress();
    
    // NOWE METODY:
    bool pressShutter();    // wciśnij spust (bez automatycznego zwalniania)
    bool releaseShutter();  // zwolnij spust

    /**
     * Ręczne rozłączenie (jeśli chcesz).
     * W tym kodzie nie jest wywoływane automatycznie.
     */
    void disconnect();

private:
    // Bity do sterowania:
    const byte BUTTON_RELEASE = 0b10000000;
    const byte BUTTON_FOCUS   = 0b01000000;
    const byte MODE_IMMEDIATE = 0b00001100;

    // UUID usług / charakterystyk
    const BLEUUID SERVICE_UUID;
    const BLEUUID PAIRING_SERVICE;
    const BLEUUID SHUTTER_CONTROL_SERVICE;

    // Obiekty BLE
    BLEClient *pclient = BLEDevice::createClient();
    ConnectivityState *pconnection_state = new ConnectivityState();
    BLERemoteService *pRemoteService = nullptr;
    BLERemoteCharacteristic *pRemoteCharacteristic_Pairing = nullptr;
    BLERemoteCharacteristic *pRemoteCharacteristic_Trigger = nullptr;

    // Inne dane
    ArduinoNvs nvs;
    bool ready_to_connect = false;
    BLEAddress camera_address = BLEAddress("");
    String device_name = "";

    void scan(unsigned int scan_duration);
    bool connect();
    bool ensureConnected();
    BLERemoteCharacteristic* getTriggerCharacteristic();
};

#endif // CANON_BLE_REMOTE_H_
