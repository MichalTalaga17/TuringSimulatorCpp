# TODO - Symulator Maszyny Turinga

## 1. Rdzeń symulacji
- [x] Zaimplementować jednotaśmową taśmę i głowicę
- [x] Zaimplementować podstawową maszynę Turinga
- [x] Dodać obsługę stanów akceptujących
- [x] Dodać ruchy `L`, `P`, `_`, `STOP`
- [x] Dodać `reset()` i licznik kroków
- [ ] Rozdzielić definicję maszyny od stanu uruchomienia
- [ ] Dodać jawny wynik wykonania: `Accepted`, `Rejected`, `StoppedByLimit`, `HaltedWithoutRule`
- [ ] Dodać limit kroków

## 2. GUI w Qt
- [ ] Skonfigurować projekt Qt
- [ ] Utworzyć główne okno aplikacji
- [ ] Dodać akcje: `Nowy`, `Otwórz`, `Zapisz`, `Waliduj`, `Uruchom`, `Krok`, `Pauza`, `Reset`
- [ ] Dodać panel stanu wykonania
- [ ] Dodać widok taśmy z zaznaczoną głowicą
- [ ] Dodać pole wejścia dla zawartości taśmy startowej
- [ ] Dodać ustawienie maksymalnej liczby kroków
- [ ] Połączyć GUI z silnikiem symulacji

## 3. Format pliku i parser
- [ ] Zaprojektować własny format pliku maszyny
- [ ] Zdefiniować sekcje: nazwa, alfabet wejściowy, alfabet taśmy, symbol pusty, stan początkowy, stany akceptujące, przejścia
- [ ] Dodać obsługę komentarzy w pliku
- [ ] Zaimplementować parser formatu
- [ ] Dodać raportowanie błędów z numerem linii
- [ ] Dodać walidację semantyczną definicji maszyny
- [ ] Dodać edytor definicji maszyny
- [ ] Dodać panel błędów walidacji
- [ ] Dodać zapis definicji do pliku
- [ ] Dodać odczyt definicji z pliku

## 4. Architektura aplikacji
- [ ] Wydzielić warstwę `core`
- [ ] Wydzielić warstwę `parser`
- [ ] Wydzielić warstwę `application`
- [ ] Przygotować interfejs pod przyszłą bibliotekę projektów / maszyn
- [ ] Zdefiniować typ `MachineDefinition`
- [ ] Zdefiniować typ `SimulationSnapshot`
- [ ] Zdefiniować typ `ValidationIssue`

## 5. Debugowanie i analiza
- [ ] Dodać historię kroków wykonania
- [ ] Dodać możliwość cofania się po historii
- [ ] Dodać podgląd zmian między krokami
- [ ] Dodać breakpointy na stanach
- [ ] Dodać breakpoint na limicie kroków
- [ ] Dodać czytelny status wykonania maszyny

## 6. Tryb eksperymentów
- [ ] Umożliwić uruchamianie jednej maszyny dla wielu wejść
- [ ] Dodać tabelę wyników dla wielu uruchomień
- [ ] Pokazywać dla każdego wejścia: wynik, liczbę kroków, końcowy stan, końcowy fragment taśmy
- [ ] Dodać możliwość szybkiego porównania wyników

## 7. Przykłady i materiały pokazowe
- [ ] Utworzyć katalog `examples/`
- [ ] Dodać kilka gotowych maszyn demonstracyjnych
- [ ] Dodać przykładowe wejścia do maszyn
- [ ] Przygotować scenariusz prezentacji projektu
- [ ] Dodać ekran pomocy z opisem formatu pliku

## 8. Testy
- [ ] Dodać testy jednostkowe dla taśmy
- [ ] Dodać testy jednostkowe dla maszyny Turinga
- [ ] Dodać testy dla zatrzymania bez reguły
- [ ] Dodać testy dla `STOP`
- [ ] Dodać testy dla limitu kroków
- [ ] Dodać testy parsera poprawnych plików
- [ ] Dodać testy parsera błędnych plików
- [ ] Dodać testy integracyjne: plik -> parser -> uruchomienie

## 9. Rozszerzenia po v1
- [ ] Dodać bibliotekę zapisanych maszyn
- [ ] Rozważyć bazę danych do przechowywania projektów i historii uruchomień
- [ ] Rozważyć eksport raportów z uruchomienia
- [ ] Rozważyć moduł AI jako v2, np. generowanie szkicu maszyny z opisu
