'use client';

import type { SolverInput, SolverResult, SolutionAssignment, SolutionMeta } from '@/types';

// ─── WASM module type declarations ───────────────────────────────────────────

interface WasmConfig {
  max_keep_solutions: number;
  max_solutions: number;
  max_runtime: number;
  verbose: boolean;
  early_stopping: boolean;
  simplified_evaluation: boolean;
  solution_callback: ((solutionJson: string) => void) | undefined;
  delete(): void;
}

interface WasmSolverRunner {
  run(inputJson: string): string;
  delete(): void;
}

interface PrzydzielaczkaModule {
  Config: new () => WasmConfig;
  SolverRunner: new (config: WasmConfig) => WasmSolverRunner;
}

declare global {
  interface Window {
    createPrzydzielaczkaModule?: () => Promise<PrzydzielaczkaModule>;
    __przydzielaczkaModule?: PrzydzielaczkaModule;
  }
}

// ─── Debug helper ─────────────────────────────────────────────────────────────

const DEBUG = process.env.NEXT_PUBLIC_DEBUG === 'true';
const dbg = (...args: unknown[]) => { if (DEBUG) console.log('[solver]', ...args); };

// ─── Solver configuration ─────────────────────────────────────────────────────

export interface SolverOptions {
  max_solutions?: number;
  max_runtime?: number;
  early_stopping?: boolean;
  onSolution?: (solution: SolutionAssignment[]) => void;
}

// ─── WASM loader ─────────────────────────────────────────────────────────────

let modulePromise: Promise<PrzydzielaczkaModule> | null = null;

export async function loadSolverModule(): Promise<PrzydzielaczkaModule> {
  if (window.__przydzielaczkaModule) return window.__przydzielaczkaModule;

  if (!modulePromise) {
    modulePromise = new Promise<PrzydzielaczkaModule>((resolve, reject) => {
      const existing = document.querySelector<HTMLScriptElement>(
        'script[data-wasm="przydzielaczka"]'
      );
      if (existing) {
        // Already injected, wait for it
        const wait = () => {
          if (window.createPrzydzielaczkaModule) {
            window.createPrzydzielaczkaModule().then(m => {
              window.__przydzielaczkaModule = m;
              resolve(m);
            });
          } else {
            setTimeout(wait, 50);
          }
        };
        wait();
        return;
      }

      const basePath = process.env.NEXT_PUBLIC_BASE_PATH ?? '';
      const wasmSrc = `${basePath}/wasm/przydzielaczka_wasm.js`;

      const script = document.createElement('script');
      script.src = wasmSrc;
      script.dataset.wasm = 'przydzielaczka';
      script.onload = () => {
        if (!window.createPrzydzielaczkaModule) {
          reject(new Error('WASM module factory not found after script load'));
          return;
        }
        window.createPrzydzielaczkaModule()
          .then(m => {
            window.__przydzielaczkaModule = m;
            resolve(m);
          })
          .catch(reject);
      };
      script.onerror = () =>
        reject(new Error(`Failed to load ${wasmSrc} — did you run "npm run copy-wasm"?`));
      document.head.appendChild(script);
    });
  }

  return modulePromise;
}

// ─── Main solver wrapper ──────────────────────────────────────────────────────

export async function runSolver(
  input: SolverInput,
  opts: SolverOptions = {}
): Promise<SolverResult[]> {
  const Module = await loadSolverModule();
  dbg('WASM module loaded');

  const config = new Module.Config();
  config.max_solutions       = opts.max_solutions  ?? 5;
  config.max_runtime         = opts.max_runtime    ?? 60;
  config.early_stopping      = opts.early_stopping ?? true;
  config.verbose             = DEBUG;
  config.max_keep_solutions  = opts.max_solutions  ?? 5;
  config.simplified_evaluation = false;
  dbg('config', { max_solutions: config.max_solutions, max_runtime: config.max_runtime, early_stopping: config.early_stopping });

  const partialResults: SolverResult[] = [];

  if (opts.onSolution) {
    config.solution_callback = (jsonStr: string) => {
      try {
        const partial: SolverResult = JSON.parse(jsonStr);
        partialResults.push(partial);
        opts.onSolution!(partial.solutions ?? []);
      } catch {
        // ignore malformed callback payloads
      }
    };
  } else {
    config.solution_callback = undefined;
  }

  // Strip UI-only fields before sending to solver
  const solverInput: SolverInput = {
    constraints: input.constraints,
    classes: input.classes.map(({ name: _n, zajCykId: _z, ...rest }) => rest),
  };

  dbg(`input: ${solverInput.classes.length} classes, ${solverInput.constraints.length} constraints`);
  dbg('classes sample (first 3):', solverInput.classes.slice(0, 3));
  dbg('constraints:', solverInput.constraints);
  dbg('full input JSON:', JSON.stringify(solverInput, null, 2));

  const runner = new Module.SolverRunner(config);
  let resultJson: string;
  try {
    resultJson = runner.run(JSON.stringify(solverInput));
  } finally {
    runner.delete();
    config.delete();
  }

  dbg('raw result JSON:', resultJson);

  const raw = JSON.parse(resultJson) as {
    meta: SolutionMeta;
    results: Array<{ chosen_classes: Array<{ id: string; class_type: string; group: number }> }>;
    summary?: unknown;
  };

  const solverResults: SolverResult[] = (raw.results ?? []).map(r => ({
    solutions: (r.chosen_classes ?? []).map(cls => ({
      class_id: cls.id,
      class_type: cls.class_type,
      group: cls.group,
    })),
    meta: raw.meta,
  }));

  dbg('parsed result — solutions:', solverResults.length, 'meta:', raw.meta);
  return solverResults;
}
