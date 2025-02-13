# Canon-R5-MKII-BLE
README – Canon BLE Remote, v1.1
Spis treści
Opis projektu
Struktura plików i folderów
Schemat działania programu
Schemat programowania i wprowadzania zmian
Schemat zmiennych i konfiguracji
Jak uruchomić i skompilować
Dalszy rozwój
Historia wersji (Changelog)


1. 
Opis projektu
Projekt pozwala sterować aparatem Canon (np. R5, R6, itp.) za pomocą ESP32 poprzez Bluetooth LE (biblioteka CanonBLERemote).
Dodatkowo umożliwia sterowanie obiektywem (np. Canon 50 mm) poprzez SPI (biblioteka Canon50mmLens).

Funkcjonalność obejmuje:

Parowanie z aparatem i zapamiętywanie adresu w NVS.
Wyzwolenie migawki (krótki impuls, focus, holdShutter, itp.).
Ręczne wciśnięcie/zwolnienie migawki (pressShutter(), releaseShutter()), co pozwala trzymać spust przez dowolny czas.
Ruch obiektywu (fokus) z możliwością sekwencji przesuwania (ContinuousFocusMinusX).
Przerwanie cyklu (stopOperation) i automatyczne zwolnienie migawki.
Konfigurację kluczowych parametrów (opóźnienia, liczba kroków) w jednym miejscu (CanonRemoteConfig.h).
Na ten moment sterowanie odbywa się przez Serial Monitor (komendy wpisywane w terminalu). W przyszłości przewidziane jest przeniesienie na wyświetlacz OLED i przyciski.

2. 
Struktura plików i folderów
Poniższe drzewo pokazuje zalecaną strukturę dla PlatformIO (ale podobnie można w innych środowiskach):


your_project/                      <-- Główny folder projektu
 ├─ platformio.ini                 <-- Konfiguracja PlatformIO
 ├─ README.md                      <-- (TEN plik!)
 ├─ include/
 │    ├─ CanonBLERemote.h         <-- Deklaracja biblioteki BLE (aparat)
 │    ├─ Canon50mmLens.h          <-- Deklaracja biblioteki SPI (obiektyw)
 │    └─ CanonRemoteConfig.h      <-- Główna konfiguracja param. (opóźnienia, sekwencje)
 ├─ src/
 │    ├─ main.cpp                  <-- Główny kod programu (setup, loop, komendy)
 │    ├─ CanonBLERemote.cpp       <-- Implementacja BLE
 │    └─ Canon50mmLens.cpp        <-- Implementacja SPI (obiektyw)
 ├─ lib/
 │    └─ ArduinoNvs/
 │         ├─ ArduinoNvs.h
 │         └─ ArduinoNvs.cpp      <-- Obsługa NVS
 └─ .pio / .vscode / inne pliki generowane przez PlatformIO
Główne pliki:

main.cpp – obsługa komend (Serial), wywołania metod z bibliotek (CanonBLERemote, Canon50mmLens).
CanonBLERemote.* – logika komunikacji BLE z aparatem (parowanie, wyzwalanie migawki, itp.).
Canon50mmLens.* – logika sterowania obiektywem (SPI).
CanonRemoteConfig.h – centralne miejsce na wszystkie parametry (czasy opóźnień, liczby kroków, itp.).
ArduinoNvs.* – pomocnicza biblioteka do zapisu/odczytu w pamięci NVS (adres aparatu).

3. 
Schemat działania programu
Inicjalizacja (w setup()):

canon_ble.init() – uruchamia BLE, wczytuje adres aparatu z NVS (jeśli zapisany).
lens.initLens() – ustawia piny SPI, resetuje stopOperation = false.
Obsługa komend:

W pętli loop(), funkcja handleSerialCommands().
Gdy wpiszesz p – parowanie z aparatem.
s – wyzwolenie migawki (krótki impuls),
f – focus (krótki impuls),
4..9 – sekwencje ruchu ostrości z jednoczesnym wciśnięciem spustu migawki (pressShutter() → ContinuousFocusMinusX → releaseShutter()).
k – przerwanie sekwencji (stopOperation = true) i zwolnienie spustu.
Operacje BLE (wewnątrz CanonBLERemote):

connect() / disconnect() – tylko w obrębie biblioteki.
pressShutter() / releaseShutter() – odpowiednio wciśnięcie i zwolnienie spustu.
Operacje SPI (wewnątrz Canon50mmLens):

MoveFocus(steps) – przesuwa ostrość.
ContinuousFocusMinusX(stepSize, totalShots) – w pętli wywołuje MoveFocus(-stepSize) wiele razy.
Jeśli stopOperation = true, pętla zostaje przerwana.
Przerwanie i powrót do stanu idle:

Wywołanie k ustawia stopOperation = true i zwalnia spust.


4. 
Schemat programowania i wprowadzania zmian
Nowe funkcje BLE (aparatu) dodawaj w CanonBLERemote.*.

Przykład: zoomTele(), startVideo(), itd.
Wewnątrz wywołuj ensureConnected(), potem pRemoteCharacteristic->writeValue(...) odpowiednimi bajtami.
Nowe funkcje obiektywu (SPI) dodawaj w Canon50mmLens.*.

Przykład: ContinuousFocusPlusX(...), MoveFocusAbsolute(pos), itd.
Możesz korzystać z stopOperation do przerwań.
Konfiguracja (opóźnienia, liczba kroków, parametry sekwencji)

Zmieniaj w CanonRemoteConfig.h.
Dzięki temu nie musisz modyfikować kodu w wielu plikach – tylko w jednym miejscu.
Obsługa UI (póki co: Serial, w przyszłości: OLED + przyciski)

Kod w main.cpp – tam reagujemy na komendy / przyciski.
Logika metod w bibliotekach się nie zmienia.
Nie zmieniaj raczej definicji SERVICE_UUID, PAIRING_SERVICE, SHUTTER_CONTROL_SERVICE (Canon BLE).

To kluczowe identyfikatory – ich modyfikacja złamie kompatybilność.


5. 
Schemat zmiennych i konfiguracji
Plik CanonRemoteConfig.h
lensMinFocusPos / lensMaxFocusPos – minimalna i maksymalna pozycja ostrości.
spiDelayUS – opóźnienie w mikrosekundach przy transferze SPI.
stepDelayMS – opóźnienie między kolejnymi krokami w ContinuousFocusMinusX().
sequences[] – tablica FocusSequenceParam, definiująca:
stepSize (ile kroków na jeden strzał)
totalShots (ile strzałów w sekwencji).
Dzięki temu w main.cpp (komendy '4'..'9') automatycznie odnoszą się do tych parametrów, bez zmian w kodzie logiki.



6. 
Jak uruchomić i skompilować
Zainstaluj PlatformIO w VS Code (lub innym środowisku).
Otwórz katalog your_project.
Zbuduj projekt (PlatformIO: Build).
Wgraj na ESP32 (PlatformIO: Upload).
Otwórz Serial Monitor (115200 baud).
Wpisz komendy (np. p, s, 4, k) i obserwuj działanie.
W trakcie parowania:

W aparacie Canon: włącz Remote Pairing (BLE).
W Serial wpisz p.
Po znalezieniu aparatu zapisze się adres do NVS.


7.
Dalszy rozwój
Przeniesienie UI z Serial na OLED i przyciski.
Zamiast Serial.available(), obsługujemy stany klawiszy i malujemy ekran.
Logika w CanonBLERemote i Canon50mmLens się nie zmieni – wystarczy zmiana w main.cpp.
Nowe sekwencje ostrości – wystarczy dopisać kolejne wpisy w sequences[] (np. FocusSequenceParam{12, 50UL} itp.).
Rozbudowa BLE (np. wideo, zooming w nowszych aparatach) – w CanonBLERemote.*.
System logów – można użyć esp_log_level_set() by ograniczyć lub rozbudować logging.


8. 

Historia wersji (Changelog)
v1.0

Podstawowa obsługa BLE (parowanie, trigger, focus),
Biblioteka ArduinoNvs do zapamiętywania adresu aparatu.
Proste sterowanie obiektywem + minimalne opóźnienia.
v1.1 (aktualna)

Dodano wspólną konfigurację w pliku CanonRemoteConfig.h (łatwiejsze zmiany parametrów).
Wprowadzono pressShutter() i releaseShutter() w CanonBLERemote (pełna kontrola spustu).
Sekwencje ostrości (ContinuousFocusMinusX) z automatycznym zwolnieniem migawki po zakończeniu/przerwaniu.
Komenda k do przerwania cyklu i zwolnienia spustu.
Uporządkowano strukturę katalogów i plików.