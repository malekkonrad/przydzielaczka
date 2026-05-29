'use client';

import { useState } from 'react';
import {
  Box, Button, FormControl, InputLabel, Select, MenuItem,
  CircularProgress, Alert, Typography, Collapse, TextField,
  IconButton, Tooltip,
} from '@mui/material';
import { Download, ExpandMore, ExpandLess, Info } from '@mui/icons-material';
import { useAppStore } from '@/store/appStore';
import { STUDY_PROGRAMS, STUDY_YEARS, SEMESTER_TYPE } from '@/lib/usos/constants';
import type { StudyYear } from '@/types';

const SEMESTERS = [1, 2, 3, 4, 5, 6, 7];

export default function ScheduleSelector() {
  const studyConfig      = useAppStore(s => s.studyConfig);
  const setStudyConfig   = useAppStore(s => s.setStudyConfig);
  const loadingCourses   = useAppStore(s => s.loadingCourses);
  const coursesError     = useAppStore(s => s.coursesError);
  const mergeCourseGroups = useAppStore(s => s.mergeCourseGroups);
  const setLoading        = useAppStore(s => s.setLoadingCourses);
  const setError          = useAppStore(s => s.setCoursesError);

  const [localConfig, setLocal] = useState(studyConfig);
  const [cookie, setCookie]     = useState('');
  const [showAdvanced, setShowAdvanced] = useState(false);

  async function handleLoad() {
    setLoading(true);
    setError(null);
    setStudyConfig(localConfig);

    const params = new URLSearchParams({
      program: localConfig.program,
      year:    localConfig.year,
      sem:     String(localConfig.semesterNumber),
    });

    const headers: Record<string, string> = {};
    if (cookie.trim()) headers['x-usos-cookie'] = cookie.trim();

    try {
      const res = await fetch(`/api/usos/schedule?${params}`, { headers });
      if (!res.ok) {
        const body = await res.json().catch(() => ({}));
        throw new Error(body.error ?? `HTTP ${res.status}`);
      }
      const data = await res.json();
      mergeCourseGroups(data);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : String(e));
    } finally {
      setLoading(false);
    }
  }

  return (
    <Box sx={{ p: 1.5, display: 'flex', flexDirection: 'column', gap: 1.5 }}>
      <Typography variant="caption" color="text.secondary" fontWeight={600} letterSpacing={0.5}>
        POBIERZ PLAN
      </Typography>

      <FormControl size="small" fullWidth>
        <InputLabel>Kierunek</InputLabel>
        <Select
          label="Kierunek"
          value={localConfig.program}
          onChange={e => setLocal(p => ({ ...p, program: e.target.value }))}
        >
          {STUDY_PROGRAMS.map(p => (
            <MenuItem key={p.code} value={p.code}>{p.name}</MenuItem>
          ))}
        </Select>
      </FormControl>

      <FormControl size="small" fullWidth>
        <InputLabel>Rok akademicki</InputLabel>
        <Select
          label="Rok akademicki"
          value={localConfig.year}
          onChange={e => setLocal(p => ({ ...p, year: e.target.value as StudyYear }))}
        >
          {STUDY_YEARS.map(y => (
            <MenuItem key={y} value={y}>{y}</MenuItem>
          ))}
        </Select>
      </FormControl>

      <FormControl size="small" fullWidth>
        <InputLabel>Semestr</InputLabel>
        <Select
          label="Semestr"
          value={localConfig.semesterNumber}
          onChange={e => setLocal(p => ({ ...p, semesterNumber: Number(e.target.value) }))}
        >
          {SEMESTERS.map(n => (
            <MenuItem key={n} value={n}>
              {n} ({SEMESTER_TYPE[n] === 'Z' ? 'Zimowy' : 'Letni'})
            </MenuItem>
          ))}
        </Select>
      </FormControl>

      {/* Advanced: cookie */}
      <Box>
        <Button
          size="small"
          color="inherit"
          sx={{ fontSize: '0.7rem', p: 0, minWidth: 0, textTransform: 'none', opacity: 0.6 }}
          endIcon={showAdvanced ? <ExpandLess fontSize="inherit" /> : <ExpandMore fontSize="inherit" />}
          onClick={() => setShowAdvanced(v => !v)}
        >
          Zaawansowane
        </Button>
        <Collapse in={showAdvanced}>
          <Box sx={{ mt: 1, display: 'flex', alignItems: 'flex-start', gap: 0.5 }}>
            <TextField
              size="small"
              fullWidth
              label="Cookie USOS"
              placeholder="PHPSESSID=abc123..."
              value={cookie}
              onChange={e => setCookie(e.target.value)}
              multiline
              minRows={2}
              inputProps={{ style: { fontSize: '0.72rem', fontFamily: 'monospace' } }}
            />
            <Tooltip title="Jeśli pobieranie planu wymaga zalogowania, wklej tutaj wartość nagłówka Cookie z DevTools (F12 → Network → dowolne żądanie do usos.agh.edu.pl → Request Headers → Cookie).">
              <IconButton size="small" sx={{ mt: 0.5 }}>
                <Info fontSize="small" />
              </IconButton>
            </Tooltip>
          </Box>
        </Collapse>
      </Box>

      <Button
        variant="contained"
        size="small"
        startIcon={loadingCourses ? <CircularProgress size={14} color="inherit" /> : <Download />}
        disabled={loadingCourses}
        onClick={handleLoad}
        fullWidth
      >
        {loadingCourses ? 'Pobieranie…' : 'Pobierz plan'}
      </Button>

      {coursesError && (
        <Alert severity="error" sx={{ fontSize: '0.75rem', py: 0.5 }}>
          {coursesError}
        </Alert>
      )}
    </Box>
  );
}
