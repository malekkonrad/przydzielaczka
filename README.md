# przydzielaczka
Przydzielaczka to system do inteligentnego przypisywania studentów do grup zajęciowych z uwzględnieniem ich preferencji, jakości prowadzących oraz ograniczeń czasowych. Projekt wykorzystuje własny solver oparty na podejściu Constraint Satisfaction Problem (CSP), rozszerzony o optymalizację miękkich ograniczeń, takich jak minimalizacja przerw, liczby godzin czy konfliktów w planie.

Rdzeń aplikacji został zaimplementowany w C++, z możliwością użycia zarówno jako biblioteka, moduł Pythona, jak i solver działający w przeglądarce dzięki WebAssembly. System umożliwia analizę różnych scenariuszy oraz ocenę jakości przydziału, oferując elastyczne dostosowanie wag dla poszczególnych kryteriów.

Celem projektu jest znalezienie możliwie najlepszego kompromisu pomiędzy wymaganiami organizacyjnymi a satysfakcją studentów.

# Wymagania Systemu

- przypisanie studenta, które jest mierzalnie dobre
- szybkie działanie solvera przy małej ilości zasobów
- działanie jako:
  - bilblioteka Cpp
  - moduł Python
  - solver w przeglądarce (WASM)
- układanie planu dla szkoły (większa liczba studentów)

# Tech stack

- nlohmanjson for Cpp json
- emscriptem for binding wasm to Cpp
- pybind 11 for binding python to Cpp
- solver csp with heuristics, backtracking, branch and bound
- next.js react for frontend

# Ograniczenia

- Minimalizuj przerwy między zajęciami
- Przerwy nie mogą wynosić mniej niż 15 minut
- Preferencje do grupy (twarde lub miękkie)
- Preferencje do prowadzących (twarde lub miękkie)
- Preferencje do wykładów (twarde lub miękkie)
- Maksymalizuj uczęszczanie na zajęcia (pojedyncze)
- Maksymalizuj uczęszczanie na zajęcia (wszystkie w planie)
- Blokowanie godzin w danych dniach (twarde lub miękkie)
- Informowanie o kolizjach między grupami 
- All good solutions (best effort)
