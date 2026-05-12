# Symulator Maszyny Turinga w C++

Prosty symulator jednotaśmowej deterministycznej maszyny Turinga napisany w C++.

Aktualnie projekt zawiera:
- rdzeń symulacji w czystym C++,
- przykładową maszynę uruchamianą w konsoli,
- notację stanów w stylu `q0`, `q1`, `q2` itd.,
- notację ruchów głowicy widoczną dla użytkownika: `L`, `P`, `_`, `STOP`.

## Wymagania

Do uruchomienia projektu na innym komputerze potrzebne są:
- `git`
- `cmake` w wersji 3.20 lub nowszej
- kompilator obsługujący C++17

Przykłady:
- macOS: `clang++` z Xcode Command Line Tools
- Linux: `g++` albo `clang++`
- Windows: MSVC albo MinGW

## Instalacja CMake

### macOS

Najpierw warto doinstalować narzędzia deweloperskie Apple:

```bash
xcode-select --install
```

Jeśli używasz Homebrew:

```bash
brew install cmake
```

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install cmake build-essential git
```

### Fedora

```bash
sudo dnf install cmake gcc-c++ git
```

### Windows

Najwygodniejsze opcje:
- zainstalować Visual Studio Community z modułem `Desktop development with C++`,
- albo pobrać `CMake` bezpośrednio ze strony producenta: [cmake.org](https://cmake.org/download/).

Po instalacji warto sprawdzić, czy polecenie działa:

```bash
cmake --version
```

## Pobranie projektu

```bash
git clone https://github.com/MichalTalaga17/TuringSimulatorCpp
cd TuringSimulatorCpp
```

## Budowanie

```bash
cmake -S . -B build
cmake --build build
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

## Aktualny przykład działania

Domyślny przykład:
- startuje w stanie `q0`,
- przesuwa głowicę w prawo po kolejnych symbolach `1`,
- po napotkaniu symbolu pustego dopisuje `1`,
- przechodzi do stanu `q1`,
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
- `src/Tape.cpp` - implementacja taśmy
- `src/TuringMachine.cpp` - implementacja maszyny
- `main.cpp` - prosty przykład uruchamiany w konsoli

## Uwagi

Na tym etapie projekt nie używa jeszcze Qt. Interfejs graficzny można dodać później, już na bazie istniejącego rdzenia symulacji.
