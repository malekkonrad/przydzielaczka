# przydzielaczka

[![Deploy to GitHub Pages Status](https://github.com/malekkonrad/przydzielaczka/actions/workflows/deploy.yml/badge.svg)](https://github.com/malekkonrad/przydzielaczka/actions/workflows/deploy.yml)
&nbsp;

[**Otwórz aplikację**](https://malekkonrad.github.io/przydzielaczka/)

Przydzielaczka to system do inteligentnego układania planu zajęć poprzez odpowiedni dobór grup zajęciowych z uwzględnieniem preferencji, oceny prowadzących oraz ograniczeń czasowych. Projekt wykorzystuje własny solver oparty na podejściu Constraint Satisfaction Problem (CSP), rozszerzony o optymalizację miękkich ograniczeń, takich jak minimalizacja przerw, liczby godzin czy konfliktów w planie.

Rdzeń aplikacji został zaimplementowany w C++, z możliwością użycia zarówno jako biblioteka, moduł Pythona, jak i solver działający w przeglądarce dzięki WebAssembly. System umożliwia analizę różnych scenariuszy oraz ocenę jakości przydziału, oferując elastyczne dostosowanie wag dla poszczególnych kryteriów.

Celem projektu jest znalezienie możliwie najlepszego kompromisu pomiędzy wymaganiami organizacyjnymi a satysfakcją studentów.

# Wymagania
- uzyskanie mierzalnie dobrych doborów zajęć
- szybkie działanie solvera przy małym zużyciu zasobów
- działanie jako:
  - bilblioteka C++
  - moduł Python
  - solver w przeglądarce (WASM) na stronie statycznej
- (OPCJONALNIE) kładanie planu dla szkoły (większa liczba studentów)

# Tech stack
- nlohmanjson json library
- emscriptem C++ - binding WASM
- pybind 11 - binding Python
- next.js static page for frontend deployed via GitHub

# Ograniczenia
- Minimalizacja przerw między zajęciami
- Ograniczenie długości przerw
- Preferencje do grupy (twarde lub miękkie)
- Preferencje do prowadzących (twarde lub miękkie)
- Preferencje do wykładów (twarde lub miękkie)
- Maksymalizacja uczęszczania na zajęcia (pojedyncze lub wszystkie w planie)
- Blokowanie godzin w danych dniach (twarde lub miękkie)
- Informacja o kolizjach między grupami 
- Możliwość, żeby dane zajęcia były na końcu lub początku dnia
