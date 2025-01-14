#include "CanonBLERemote.h"
#include <esp_log.h>

static const char *LOG_TAG = "CANON_BLE";

// ------------------------------
//       advdCallback
// ------------------------------
advdCallback::advdCallback(BLEUUID service_uuid, bool *ready_to_connect, BLEAddress *address_to_connect) {
    service_uuid_wanted = service_uuid;
    pready_to_connect = ready_to_connect;
    paddress_to_connect = address_to_connect;
}

void advdCallback::onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID()) {
        if (service_uuid_wanted.equals(advertisedDevice.getServiceUUID())) {
            *paddress_to_connect = advertisedDevice.getAddress();
            *pready_to_connect = true;
            advertisedDevice.getScan()->stop();
        }
    }
}

// ------------------------------
//       ConnectivityState
// ------------------------------
void ConnectivityState::onConnect(BLEClient *pclient) {
    connected = true;
    ESP_LOGI(LOG_TAG, "onConnect - polaczenie zestawione");
}

void ConnectivityState::onDisconnect(BLEClient *pclient) {
    connected = false;
    ESP_LOGI(LOG_TAG, "onDisconnect - polaczenie zerwane");
}

bool ConnectivityState::isConnected() {
    return connected;
}

// ------------------------------
//       CanonBLERemote
// ------------------------------
CanonBLERemote::CanonBLERemote(String name)
: SERVICE_UUID("00050000-0000-1000-0000-d8492fffa821"),
  PAIRING_SERVICE("00050002-0000-1000-0000-d8492fffa821"),
  SHUTTER_CONTROL_SERVICE("00050003-0000-1000-0000-d8492fffa821")
{
    device_name = name;
    pclient->setClientCallbacks(pconnection_state);
}

void CanonBLERemote::init() {
    BLEDevice::init(device_name.c_str());
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);

    if (nvs.begin()) {
        String address = nvs.getString("cameraaddr");
        if (address.length() == 17) {
            camera_address = BLEAddress(address.c_str());
            ESP_LOGI(LOG_TAG, "Wczytano adres z NVS: %s", address.c_str());
        } else {
            ESP_LOGI(LOG_TAG, "Brak aparatu w NVS");
        }
    } else {
        ESP_LOGE(LOG_TAG, "NVS init FAIL");
    }
}

bool CanonBLERemote::pair(unsigned int scan_duration) {
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
    ready_to_connect = false;

    // Skanuj przez podaną liczbę sekund (domyślnie 15)
    scan(scan_duration);

    unsigned long start_ms = millis();
    while (!ready_to_connect && (millis() - start_ms < scan_duration * 1000UL)) {
        delay(10);
    }

    if (!ready_to_connect) {
        ESP_LOGI(LOG_TAG, "Nie znaleziono aparatu w poblizu (scan=%d s).", scan_duration);
        return false;
    }

    ESP_LOGI(LOG_TAG, "Znaleziono urzadzenie: %s. Proba parowania...", camera_address.toString().c_str());

    if (!connect()) {
        ESP_LOGE(LOG_TAG, "Nie udalo sie polaczyc (pair).");
        return false;
    }

    // Główna usługa
    pRemoteService = pclient->getService(SERVICE_UUID);
    if (!pRemoteService) {
        ESP_LOGE(LOG_TAG, "Brak glownej uslugi BLE w aparacie");
        disconnect();
        return false;
    }

    // Charakterystyka parowania
    pRemoteCharacteristic_Pairing = pRemoteService->getCharacteristic(PAIRING_SERVICE);
    if (!pRemoteCharacteristic_Pairing) {
        ESP_LOGE(LOG_TAG, "Brak charakterystyki parowania w aparacie");
        disconnect();
        return false;
    }

    // Wysłanie „żądania parowania”
    String device_name_ = " " + device_name + " ";
    byte cmdPress[device_name_.length()];
    device_name_.getBytes(cmdPress, device_name_.length());
    cmdPress[0] = 0x03;

    pRemoteCharacteristic_Pairing->writeValue(cmdPress, sizeof(cmdPress), false);
    ESP_LOGI(LOG_TAG, "Parowanie zakonczone - success (wyslano request). Rozlaczamy i przechodzimy na NO_MITM.");

    disconnect();
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);

    // Ponowne łączenie bez MITM
    if (!connect()) {
        ESP_LOGE(LOG_TAG, "Nie udalo sie polaczyc ponownie po pair()");
        return false;
    }

    // Zapis w NVS
    nvs.setString("cameraaddr", String(camera_address.toString().c_str()));
    nvs.commit();
    ESP_LOGI(LOG_TAG, "Adres aparatu zapisany w NVS: %s", camera_address.toString().c_str());

    // Rozłączamy się TYLKO na końcu parowania
    disconnect();
    return true;
}

bool CanonBLERemote::isConnected() {
    // Ta metoda zwraca true, jeśli jest fizyczne połączenie BLE
    return pconnection_state->isConnected();
}

bool CanonBLERemote::trigger() {
    if (!ensureConnected()) {
        ESP_LOGE(LOG_TAG, "trigger() - brak polaczenia");
        return false;
    }
    BLERemoteCharacteristic* pChar = getTriggerCharacteristic();
    if (!pChar) {
        disconnect();
        return false;
    }

    // Krótki impuls
    byte cmdByte = (MODE_IMMEDIATE | BUTTON_RELEASE);
    pChar->writeValue(&cmdByte, 1, false);
    // zmniejszamy opóźnienie z 200 do 70 ms (możesz dalej zmniejszać w razie potrzeby)
    delay(70);

    byte releaseByte = MODE_IMMEDIATE;
    pChar->writeValue(&releaseByte, 1, false);
    // zmniejszamy z 50 do 20 ms
    delay(20);

    // Domyślnie – NIE rozłączamy się, by "isConnected()" = true
    // Możesz odkomentować, jeśli chcesz zachowania 'rozłącz się zaraz po' 
    // disconnect();
    return true;
}

bool CanonBLERemote::focus() {
    if (!ensureConnected()) {
        ESP_LOGE(LOG_TAG, "focus() - brak polaczenia");
        return false;
    }
    BLERemoteCharacteristic* pChar = getTriggerCharacteristic();
    if (!pChar) {
        disconnect();
        return false;
    }

    byte cmdByte = (MODE_IMMEDIATE | BUTTON_FOCUS);
    pChar->writeValue(&cmdByte, 1, false);
    delay(70);

    byte releaseByte = MODE_IMMEDIATE;
    pChar->writeValue(&releaseByte, 1, false);
    delay(20);

    // Również nie rozłączamy 
    // disconnect();
    return true;
}

bool CanonBLERemote::holdShutter(unsigned long holdTimeMs) {
    if (!ensureConnected()) {
        ESP_LOGE(LOG_TAG, "holdShutter() - brak polaczenia");
        return false;
    }

    BLERemoteCharacteristic* pChar = getTriggerCharacteristic();
    if (!pChar) {
        disconnect();
        return false;
    }

    // Wciśnij
    byte pressByte = (MODE_IMMEDIATE | BUTTON_RELEASE);
    pChar->writeValue(&pressByte, 1, false);
    ESP_LOGI(LOG_TAG, "holdShutter: wcisnieto spust, czekam %lu ms...", holdTimeMs);

    unsigned long start = millis();
    while (millis() - start < holdTimeMs) {
        delay(50);
        if (!isConnected()) {
            ESP_LOGE(LOG_TAG, "Utracono polaczenie w trakcie holdShutter!");
            return false;
        }
    }

    // Zwolnij
    byte releaseByte = MODE_IMMEDIATE;
    pChar->writeValue(&releaseByte, 1, false);

    // Ewentualnie rozłącz:
    // disconnect();
    return true;
}

String CanonBLERemote::getPairedAddressString() {
    return String(camera_address.toString().c_str());
}

void CanonBLERemote::eraseSavedAddress() {
    if (nvs.begin()) {
        // Najprościej: skasuj klucz "cameraaddr" przez erase() lub ustaw pusty string:
        if (!nvs.erase("cameraaddr", true)) {
            ESP_LOGW(LOG_TAG, "Nie bylo klucza cameraaddr w NVS albo blad erase()");
        } else {
            ESP_LOGI(LOG_TAG, "Skasowano klucz cameraaddr w NVS");
        }
    } else {
        ESP_LOGE(LOG_TAG, "NVS init FAIL (eraseSavedAddress)");
    }
    // Wyzeruj w pamięci:
    camera_address = BLEAddress("00:00:00:00:00:00");
}

void CanonBLERemote::scan(unsigned int scan_duration) {
    ESP_LOGI(LOG_TAG, "Start BLE scan (scan=%ds) for Canon Service", scan_duration);
    BLEScan *pBLEScan = BLEDevice::getScan();
    advdCallback *advert_dev_callback = new advdCallback(SERVICE_UUID, &ready_to_connect, &camera_address);

    pBLEScan->setAdvertisedDeviceCallbacks(advert_dev_callback);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(scan_duration);
}

bool CanonBLERemote::connect() {
    if (camera_address.toString() == "00:00:00:00:00:00") {
        ESP_LOGW(LOG_TAG, "connect() - brak adresu aparatu");
        return false;
    }
    ESP_LOGI(LOG_TAG, "Proba connect() z: %s", camera_address.toString().c_str());
    if (pclient->connect(camera_address)) {
        ESP_LOGI(LOG_TAG, "connect() - polaczono (BLE ok)");
        pRemoteService = pclient->getService(SERVICE_UUID);
        if (pRemoteService) {
            return true;
        }
        ESP_LOGE(LOG_TAG, "connect() - brak service_uuid");
        disconnect();
    } else {
        ESP_LOGE(LOG_TAG, "connect() - pclient->connect() zwrocilo false");
    }
    return false;
}

bool CanonBLERemote::ensureConnected() {
    if (isConnected()) {
        return true;
    }
    return connect();
}

void CanonBLERemote::disconnect() {
    if (pclient->isConnected()) {
        pclient->disconnect();
    }
}

BLERemoteCharacteristic* CanonBLERemote::getTriggerCharacteristic() {
    if (!pRemoteService) {
        ESP_LOGE(LOG_TAG, "getTriggerCharacteristic() - pRemoteService=nullptr");
        return nullptr;
    }
    pRemoteCharacteristic_Trigger = pRemoteService->getCharacteristic(SHUTTER_CONTROL_SERVICE);
    if (!pRemoteCharacteristic_Trigger) {
        ESP_LOGE(LOG_TAG, "getTriggerCharacteristic() - nie znaleziono SHUTTER_CONTROL_SERVICE");
    }
    return pRemoteCharacteristic_Trigger;
}
