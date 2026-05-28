'use client';

import { useState, useCallback } from 'react';
import {
  Box, Button, Typography, LinearProgress, Alert, Divider,
  TextField, FormControlLabel, Switch, Tooltip, Stack,
  CircularProgress,
} from '@mui/material';
import { PlayArrow, Science } from '@mui/icons-material';
import { useAppStore } from '@/store/appStore';
import { runSolver } from '@/lib/solver/wasmWrapper';
import type { SolverInput, SolverRun } from '@/types';
import ConstraintList from './ConstraintList';
import SolutionSelector from './SolutionSelector';
import format from 'date-fns/format';

export default function SolverPanel() {
  const constraints      = useAppStore(s => s.constraints);
  const getSelectedClasses = useAppStore(s => s.getSelectedClasses);
  const solverRunning    = useAppStore(s => s.solverRunning);
  const setSolverRunning = useAppStore(s => s.setSolverRunning);
  const addSolverRun     = useAppStore(s => s.addSolverRun);
  const setActiveSolverRunId = useAppStore(s => s.setActiveSolverRunId);

  const [maxSolutions, setMaxSolutions]   = useState(5);
  const [maxRuntime,   setMaxRuntime]     = useState(30);
  const [earlyStopping, setEarlyStopping] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [progress, setProgress] = useState(0);

  const DEBUG = process.env.NEXT_PUBLIC_DEBUG === 'true';

  const handleSolve = useCallback(async () => {
    const classes = getSelectedClasses();
    if (DEBUG) console.log('[SolverPanel] getSelectedClasses():', classes.length, 'classes', classes);
    if (classes.length === 0) {
      setError('Brak zaznaczonych zajęć.');
      return;
    }

    setSolverRunning(true);
    setError(null);
    setProgress(0);

    const input: SolverInput = { constraints, classes };
    if (DEBUG) console.log('[SolverPanel] input.constraints:', constraints);
    const runId = crypto.randomUUID();

    try {
      const results = await runSolver(input, {
        max_solutions: maxSolutions,
        max_runtime:   maxRuntime,
        early_stopping: earlyStopping,
        onSolution: () => setProgress(p => p + 1),
      });

      if (DEBUG) console.log('[SolverPanel] results:', results);
      const run: SolverRun = {
        id: runId,
        label: `Run ${format(new Date(), 'HH:mm:ss')}`,
        timestamp: new Date(),
        input,
        results,
      };
      addSolverRun(run);
      setActiveSolverRunId(runId);
    } catch (e: unknown) {
      const msg = e instanceof Error ? e.message : String(e);
      setError(msg);
    } finally {
      setSolverRunning(false);
    }
  }, [constraints, getSelectedClasses, maxSolutions, maxRuntime, earlyStopping]);

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%' }}>
      {/* Constraints */}
      <ConstraintList />
      <Divider />

      {/* Solver config */}
      <Box sx={{ px: 1.5, py: 1.5, display: 'flex', flexDirection: 'column', gap: 1.5 }}>
        <Typography variant="caption" color="text.secondary" fontWeight={600} letterSpacing={0.5}>
          SOLVER
        </Typography>

        <Stack direction="row" spacing={1}>
          <TextField
            label="Max planów"
            type="number"
            size="small"
            value={maxSolutions}
            onChange={e => setMaxSolutions(Math.max(1, Number(e.target.value)))}
            sx={{ flex: 1 }}
            inputProps={{ min: 1, max: 20 }}
          />
          <TextField
            label="Max czas (s)"
            type="number"
            size="small"
            value={maxRuntime}
            onChange={e => setMaxRuntime(Math.max(1, Number(e.target.value)))}
            sx={{ flex: 1 }}
            inputProps={{ min: 1, max: 300 }}
          />
        </Stack>

        <FormControlLabel
          control={
            <Switch
              checked={earlyStopping}
              onChange={e => setEarlyStopping(e.target.checked)}
              size="small"
            />
          }
          label={<Typography variant="caption">Early stopping</Typography>}
        />

        {solverRunning && (
          <Box>
            <LinearProgress sx={{ borderRadius: 1 }} />
            <Typography variant="caption" color="text.secondary" sx={{ mt: 0.5, display: 'block' }}>
              Znaleziono {progress} plan{progress !== 1 ? 'ów' : ''}…
            </Typography>
          </Box>
        )}

        {error && (
          <Alert severity="error" sx={{ fontSize: '0.72rem', py: 0 }}>
            {error}
          </Alert>
        )}

        <Tooltip title="Solver działa w przeglądarce przez WebAssembly. Wymaga pliku /wasm/przydzielaczka_wasm.js">
          <Button
            variant="contained"
            color="secondary"
            startIcon={solverRunning ? <CircularProgress size={16} color="inherit" /> : <PlayArrow />}
            disabled={solverRunning}
            onClick={handleSolve}
            fullWidth
          >
            {solverRunning ? 'Rozwiązywanie…' : 'Uruchom solver'}
          </Button>
        </Tooltip>

        <Typography variant="caption" color="text.secondary" sx={{ textAlign: 'center', fontSize: '0.65rem' }}>
          <Science sx={{ fontSize: 10, mr: 0.3 }} />
          WASM · solver_core v0.0.1
        </Typography>
      </Box>

      <Divider />

      {/* Solutions */}
      <Box sx={{ flex: 1, overflowY: 'auto' }}>
        <SolutionSelector />
      </Box>
    </Box>
  );
}
