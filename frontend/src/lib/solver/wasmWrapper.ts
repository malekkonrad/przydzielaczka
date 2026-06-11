'use client';

import type { SolverInput, SolverResult, SolutionAssignment, SolutionMeta } from '@/types';

export interface SolverOptions {
  max_solutions?: number;
  max_runtime?: number;
  early_stopping?: boolean;
  onSolution?: (solution: SolutionAssignment[]) => void;
}

const DEBUG = process.env.NEXT_PUBLIC_DEBUG === 'true';

// ─── Worker singleton ─────────────────────────────────────────────────────────

let _worker: Worker | null = null;

type PendingRun = {
  resolve: (results: SolverResult[]) => void;
  reject: (err: Error) => void;
  onSolution?: (solution: SolutionAssignment[]) => void;
};

const pending = new Map<string, PendingRun>();

function getWorker(): Worker {
  if (_worker) return _worker;

  const basePath = process.env.NEXT_PUBLIC_BASE_PATH ?? '';
  _worker = new Worker(`${basePath}/solver.worker.js`);

  _worker.onmessage = ({ data }) => {
    const run = pending.get(data.id);
    if (!run) return;

    if (data.type === 'solution') {
      run.onSolution?.(data.partial?.solutions ?? []);
    } else if (data.type === 'done') {
      pending.delete(data.id);
      try {
        const raw = JSON.parse(data.resultJson) as {
          meta: SolutionMeta;
          results: Array<{ chosen_classes: Array<{ id: string; class_type: string; group: number }> }>;
        };
        const results: SolverResult[] = (raw.results ?? []).map(r => ({
          solutions: (r.chosen_classes ?? []).map(c => ({
            class_id: c.id,
            class_type: c.class_type,
            group: c.group,
          })),
          meta: raw.meta,
        }));
        run.resolve(results);
      } catch (e) {
        run.reject(e instanceof Error ? e : new Error(String(e)));
      }
    } else if (data.type === 'error') {
      pending.delete(data.id);
      run.reject(new Error(data.message));
    }
  };

  _worker.onerror = (e) => {
    const err = new Error(`Solver worker error: ${e.message}`);
    for (const run of pending.values()) run.reject(err);
    pending.clear();
    _worker = null;
  };

  return _worker;
}

// ─── Cancel ───────────────────────────────────────────────────────────────────

export const CANCEL_MESSAGE = 'SOLVER_CANCELLED';

export function cancelSolver(): void {
  if (!_worker) return;
  _worker.terminate();
  _worker = null;
  const err = new Error(CANCEL_MESSAGE);
  for (const run of pending.values()) run.reject(err);
  pending.clear();
}

export function isCancelError(e: unknown): boolean {
  return e instanceof Error && e.message === CANCEL_MESSAGE;
}

// ─── Public API ───────────────────────────────────────────────────────────────

export async function runSolver(
  input: SolverInput,
  opts: SolverOptions = {}
): Promise<SolverResult[]> {
  const w = getWorker();
  const id = crypto.randomUUID();
  const basePath = process.env.NEXT_PUBLIC_BASE_PATH ?? '';
  const wasmJsUrl = `${window.location.origin}${basePath}/wasm/przydzielaczka_wasm.js`;

  const solverInput: SolverInput = {
    constraints: input.constraints,
    classes: input.classes.map(({ name: _n, zajCykId: _z, ...rest }) => rest),
  };

  if (DEBUG) console.log('[solver] run id:', id, 'classes:', solverInput.classes.length, 'constraints:', solverInput.constraints.length);

  return new Promise((resolve, reject) => {
    pending.set(id, { resolve, reject, onSolution: opts.onSolution });
    w.postMessage({
      type: 'run',
      id,
      wasmJsUrl,
      input: solverInput,
      config: {
        max_solutions:  opts.max_solutions  ?? 5,
        max_runtime:    opts.max_runtime    ?? 60,
        early_stopping: opts.early_stopping ?? true,
        verbose: DEBUG,
      },
    });
  });
}