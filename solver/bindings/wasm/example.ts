/**
 * Example usage of the przydzielaczka WebAssembly bindings from TypeScript/Node.js.
 *
 * Build the WASM module first (requires Emscripten):
 *   emcmake cmake .. -DBUILD_WASM_BINDINGS=ON
 *   emmake make
 *
 * Run with ts-node (or compile to JS first):
 *   npx ts-node example.ts
 */

// Type declarations for the generated WASM module.
interface SolutionAssignment {
    class_id:   string;
    class_type: string;
    group:      number | null;
}

interface SolutionMeta {
    solver_version:          string;
    solved_at:               string;
    duration_ms:             number;
    algorithm:               string;
    input_classes_total:     number;
    input_constraints_total: number;
}

interface SolverResult {
    solutions: SolutionAssignment[];
    meta:      SolutionMeta;
}

interface Config {
    max_keep_solutions:    number;
    max_solutions:         number;
    max_runtime:           number;
    verbose:               boolean;
    early_stopping:        boolean;
    simplified_evaluation: boolean;
    solution_callback:     ((solution: SolutionAssignment[]) => void) | undefined;
    delete(): void;
}

interface SolverRunner {
    run(inputJson: string): string;
    runFile(path: string):  string;
    delete(): void;
}

interface PrzydzielaczkaModule {
    Config:       new () => Config;
    SolverRunner: new (config: Config) => SolverRunner;
}

declare function createPrzydzielaczkaModule(): Promise<PrzydzielaczkaModule>;

// ---------- main ----------

async function main(): Promise<void> {
    // Load the WASM module produced by the build.
    // eslint-disable-next-line @typescript-eslint/no-var-requires
    const createModule: () => Promise<PrzydzielaczkaModule> = require("../../../cmake-build-wasm-release/solver/bindings/wasm/przydzielaczka_wasm.js");
    const Module = await createModule();

    const config = new Module.Config();
    config.max_solutions = 5;
    config.max_runtime   = 30;   // seconds
    config.verbose       = false;

    // Called once per solution found — receives a parsed JS object.
    config.solution_callback = (solution) => {
        console.log(`  [callback] solution found — ${Array.isArray(solution) ? solution.length : "?"} assignment(s)`);
    };

    const runner = new Module.SolverRunner(config);

    // --- Run from JSON string ---
    const inputJson = require("fs").readFileSync(
        require("path").resolve(__dirname, "../../../tests/data/classes_2526_L_ISI.json"),
        "utf-8"
    );

    console.log("Running solver...");
    const resultJson: string = runner.run(inputJson);
    const result: SolverResult = JSON.parse(resultJson);

    console.log(`  solutions : ${result.solutions?.length ?? 0}`);
    console.log(`  duration  : ${result.meta?.duration_ms ?? "?"} ms`);
    console.log(JSON.stringify(result, null, 2));

    // Always call delete() on Emscripten-managed objects to free C++ memory.
    runner.delete();
    config.delete();
}

main().catch(console.error);