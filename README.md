# Symulator Maszyny Turinga w C++

Prosty symulator jednotaśmowej deterministycznej maszyny Turinga napisany w C++.

Aktualnie projekt zawiera:
- rdzeń symulacji w czystym C++,
- graficzny interfejs Qt Widgets z kartami edytora,
- zapis i odczyt maszyn z plików JSON,
- zapis i odczyt maszyn z lokalnej bazy SQLite,
- generowanie definicji maszyn przez Gemini API,
- notację stanów w stylu `q0`, `q1`, `q2` itd.,
- notację ruchów głowicy widoczną dla użytkownika: `L`, `P`, `_`, `STOP`.


## Pobranie projektu

```bash
https://github.com/MichalTalaga17/TuringSimulatorCpp/releases
```

## Budowanie

```bash
cmake -S . -B build
cmake --build build
```

## Testy

```bash
ctest --test-dir build --output-on-failure
```

## Dokumentacja Doxygen

Projekt zawiera konfigurację `Doxyfile` oraz stronę główną dokumentacji w `docs/mainpage.dox`.

Generowanie bezpośrednio przez Doxygen:

```bash
doxygen Doxyfile
```

Jeśli CMake znajdzie program `doxygen`, dostępny jest też target:

```bash
cmake --build build --target docs
```

Wynik pojawi się w katalogu:

```text
docs/doxygen/html
```

## Uruchamianie

### macOS i Linux

```bash
./build/TuringSimulatorCpp
```

### Windows

W zależności od generatora i konfiguracji:

```powershell
.\build\Debug\TuringSimulatorCpp.exe
```

albo:

```powershell
.\build\Release\TuringSimulatorCpp.exe
```

## Generowanie maszyn z Gemini AI

W edytorze dostępny jest przycisk `AI`, a na stronie startowej opcja `Start z AI`.
Aplikacja poprosi o opis maszyny i wygeneruje tabelę przejść przez Gemini API.

Klucze API ustawisz w menu `Maszyna -> Ustawienia Gemini API...`.
Aplikacja przechowuje osobny klucz dla `Free tier` i osobny klucz dla `Pro tier`.
Możesz też wcześniej ustawić zmienne środowiskowe:

```bash
export GEMINI_FREE_API_KEY="twój-klucz-free"
export GEMINI_PRO_API_KEY="twój-klucz-pro"
```

Przed generowaniem aplikacja pokazuje jedno okno z opisem zadania, wyborem tieru
oraz opcją myślenia. `Pro tier` używa modelu `gemini-3.5-flash`. `Free tier`
najpierw używa `gemini-3.5-flash`, a jeśli model jest niedostępny lub limitowany,
automatycznie próbuje fallbacku `gemini-3.1-flash-lite`.

Aplikacja pyta również, czy włączyć myślenie modelu. Dla modeli Gemini 3 wysyła
`thinkingLevel`: `high` albo `minimal`.

## Przykład startowy

Domyślny przykład:
- startuje w stanie `q0`,
- zamienia kolejne symbole `0` na `1` i `1` na `0`,
- przesuwa głowicę w prawo,
- po napotkaniu symbolu pustego wykonuje `STOP`,
- kończy działanie ruchem `STOP`.

## Znaczenie ruchów głowicy

- `L` - ruch w lewo
- `P` - ruch w prawo
- `_` - brak ruchu
- `STOP` - zatrzymanie maszyny

## Struktura projektu

Najważniejsze pliki:
- `include/Tape.h` - model taśmy
- `include/TuringMachine.h` - logika maszyny Turinga
- `include/MachineEditorTab.h` - karta edycji i symulacji maszyny
- `include/TuringSimulatorUI.h` - główne okno aplikacji
- `include/storage.h` - API zapisu do JSON i SQLite
- `src/ui/` - implementacja interfejsu Qt
- `src/storage/` - implementacja zapisu i odczytu
- `tests/` - testy rdzenia symulacji
