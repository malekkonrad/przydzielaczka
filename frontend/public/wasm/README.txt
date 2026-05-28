Skopiuj tutaj pliki WASM po zbudowaniu solvera:

  cmake --preset wasm-release-preset
  cmake --build cmake-build-wasm --target przydzielaczka_wasm

  cp ../cmake-build-wasm/solver/bindings/wasm/przydzielaczka_wasm.js .
  cp ../cmake-build-wasm/solver/bindings/wasm/przydzielaczka_wasm.wasm .

Lub uruchom: npm run copy-wasm (z katalogu frontend/)


 Co zostało zrobione

  frontend/ — Next.js 15 + TypeScript + MUI v6

  Struktura (21 plików)

  app/
    layout.tsx            root z MUI ThemeProvider + CSS react-big-calendar
    page.tsx              → AppLayout
    providers.tsx         dark/light theme
    api/usos/
      schedule/           scraper plan grupy przedmiotów (server-side)
      class-details/      daty konkretnych sesji (server-side)

  src/
    types/index.ts        wszystkie typy (solver + UI)
    lib/usos/parser.ts    parsowanie HTML USOSweb (cheerio)
    lib/usos/constants.ts programy studiów, kolory, mapowania
    lib/solver/wasmWrapper.ts  loader WASM + runSolver()
    store/appStore.ts     Zustand state (persist: motyw, config, constraints)
    theme/theme.ts        MUI dark/light theme builder
    components/
      layout/             AppBar (tryb/ciemny jasny) + AppLayout (3-kolumny)
      sidebar/            ScheduleSelector + CourseList (z kolorowymi checkboxami)
      calendar/           TimetableCalendar (react-big-calendar, widok wzorzec/terminy)
      solver/             SolverPanel + ConstraintList + ConstraintEditor + SolutionSelector

  Uruchomienie

  cd frontend
  npm install
  npm run dev        # http://localhost:3000

  WASM solver

  Najpierw zbuduj solver i skopiuj pliki:
  cmake --preset wasm-release-preset
  cmake --build cmake-build-wasm --target przydzielaczka_wasm
  cd frontend
  npm run copy-wasm

  Przepływ działania

  1. Wybierz kierunek/rok/semestr → Pobierz plan (scraper USOS po stronie serwera)
  2. Zaznacz/odznacz przedmioty w sidebarze
  3. Przełącz widok: Wzorzec (syntetyczny tydzień) / Terminy (realne daty)
  4. Dodaj ograniczenia w prawym panelu
  5. Uruchom solver → wyniki pojawią się na kalendarzu jako osobna warstwa
  6. Przełączaj między wynikami solvera w sekcji "Wyniki solvera"
