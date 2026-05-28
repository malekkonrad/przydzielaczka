'use client';

import {
  Box, Typography, Accordion, AccordionSummary, AccordionDetails,
  ToggleButton, ToggleButtonGroup, Chip, IconButton, Tooltip,
  Stack,
} from '@mui/material';
import { ExpandMore, Visibility, VisibilityOff, Delete } from '@mui/icons-material';
import { useAppStore } from '@/store/appStore';
import type { SolverRun } from '@/types';
import format from 'date-fns/format';

export default function SolutionSelector() {
  const solverRuns         = useAppStore(s => s.solverRuns);
  const activeSolverRunId  = useAppStore(s => s.activeSolverRunId);
  const setActiveSolverRunId = useAppStore(s => s.setActiveSolverRunId);
  const activeSolIdxMap    = useAppStore(s => s.activeSolutionIndex);
  const setActiveSolIdx    = useAppStore(s => s.setActiveSolutionIndex);

  if (solverRuns.length === 0) return null;

  return (
    <Box>
      <Typography variant="caption" color="text.secondary" fontWeight={600} letterSpacing={0.5}
        sx={{ px: 1.5, pt: 1, pb: 0.5, display: 'block' }}>
        WYNIKI SOLVERA ({solverRuns.length})
      </Typography>

      {solverRuns.map(run => {
        const isActive = activeSolverRunId === run.id;
        const solutionCount = run.results.length;
        const activeSolIdx = activeSolIdxMap[run.id] ?? 0;
        const solResult = run.results[activeSolIdx];

        return (
          <Accordion key={run.id} disableGutters elevation={0}
            sx={{ border: '1px solid', borderColor: 'divider', mb: 0.5, mx: 1 }}>
            <AccordionSummary expandIcon={<ExpandMore />} sx={{ minHeight: 36, '& .MuiAccordionSummary-content': { my: 0.5 } }}>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ width: '100%', mr: 1 }}>
                <Typography variant="body2" sx={{ fontSize: '0.75rem', fontWeight: 500, flex: 1 }}>
                  {run.label}
                </Typography>
                <Chip
                  label={`${solutionCount} plan${solutionCount !== 1 ? 'ów' : ''}`}
                  size="small"
                  color={solutionCount > 0 ? 'success' : 'default'}
                  sx={{ height: 16, fontSize: '0.6rem' }}
                />
                <Tooltip title={isActive ? 'Ukryj na kalendarzu' : 'Pokaż na kalendarzu'}>
                  <IconButton
                    size="small"
                    component="div"
                    role="button"
                    onClick={e => { e.stopPropagation(); setActiveSolverRunId(isActive ? null : run.id); }}
                  >
                    {isActive ? <Visibility fontSize="small" color="primary" /> : <VisibilityOff fontSize="small" />}
                  </IconButton>
                </Tooltip>
              </Stack>
            </AccordionSummary>

            <AccordionDetails sx={{ px: 1, py: 0.5 }}>
              <Typography variant="caption" color="text.secondary" sx={{ display: 'block', mb: 0.5 }}>
                {format(run.timestamp, 'HH:mm:ss')} ·{' '}
                {solResult?.meta?.duration_ms ?? 0} ms ·{' '}
                algorytm: {solResult?.meta?.algorithm ?? '?'}
              </Typography>

              {solutionCount > 1 && (
                <ToggleButtonGroup
                  size="small" exclusive
                  value={activeSolIdx}
                  onChange={(_, v) => v != null && setActiveSolIdx(run.id, v)}
                  sx={{ mb: 0.5, flexWrap: 'wrap', gap: 0.3 }}
                >
                  {run.results.map((_, i) => (
                    <ToggleButton key={i} value={i}
                      sx={{ py: 0, px: 0.8, fontSize: '0.6rem', minWidth: 28, lineHeight: 1.8 }}>
                      {i + 1}
                    </ToggleButton>
                  ))}
                </ToggleButtonGroup>
              )}

              {solResult && solResult.solutions.length > 0 && (
                <Box sx={{ maxHeight: 120, overflowY: 'auto' }}>
                  {solResult.solutions.map((a, i) => (
                    <Box key={i} sx={{ display: 'flex', gap: 0.5, flexWrap: 'wrap', mb: 0.3 }}>
                      <Chip label={a.class_id} size="small" sx={{ height: 16, fontSize: '0.58rem' }} />
                      <Chip label={a.class_type} size="small" variant="outlined" sx={{ height: 16, fontSize: '0.58rem' }} />
                      <Chip label={`gr.${a.group ?? '?'}`} size="small" color="primary" sx={{ height: 16, fontSize: '0.58rem' }} />
                    </Box>
                  ))}
                </Box>
              )}
            </AccordionDetails>
          </Accordion>
        );
      })}
    </Box>
  );
}
