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
import { fetchScheduleData, IS_STATIC } from '@/lib/scheduleSource';
import type { StudyYear } from '@/types';
import coveredMajorsRaw from '../../../covered_majors.json';

type CoveredProgram = {
  program: string;
  name?: string;
  groupCodePrefix: string;
  years: string[];
  sems: number[];
};
const coveredMajors: CoveredProgram[] = coveredMajorsRaw as CoveredProgram[];

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

  // In static mode options come from covered_majors.json; otherwise use all constants
  const availablePrograms = IS_STATIC
    ? STUDY_PROGRAMS.filter(p => coveredMajors.some(m => m.program === p.code))
    : STUDY_PROGRAMS;

  const coveredEntry = coveredMajors.find(m => m.program === localConfig.program);

  const availableYears = IS_STATIC
    ? (coveredEntry?.years ?? [])
    : STUDY_YEARS;

  const availableSemesters = IS_STATIC
    ? (coveredEntry?.sems ?? [])
    : SEMESTERS;

  function handleProgramChange(program: string) {
    if (IS_STATIC) {
      const entry = coveredMajors.find(m => m.program === program);
      const years = entry?.years ?? [];
      const year = years.includes(localConfig.year) ? localConfig.year : (years[0] ?? localConfig.year);
      const sems = entry?.sems ?? [];
      const sem = sems.includes(localConfig.semesterNumber) ? localConfig.semesterNumber : (sems[0] ?? localConfig.semesterNumber);
      setLocal(p => ({ ...p, program, year: year as StudyYear, semesterNumber: sem }));
    } else {
      setLocal(p => ({ ...p, program }));
    }
  }

  function handleYearChange(year: StudyYear) {
    if (IS_STATIC) {
      const sems = coveredEntry?.sems ?? [];
      const sem = sems.includes(localConfig.semesterNumber) ? localConfig.semesterNumber : (sems[0] ?? localConfig.semesterNumber);
      setLocal(p => ({ ...p, year, semesterNumber: sem }));
    } else {
      setLocal(p => ({ ...p, year }));
    }
  }

  async function handleLoad() {
    setLoading(true);
    setError(null);
    setStudyConfig(localConfig);

    try {
      const data = await fetchScheduleData(
        localConfig.program,
        localConfig.year,
        localConfig.semesterNumber,
        IS_STATIC ? undefined : (cookie.trim() || undefined),
      );
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
          onChange={e => handleProgramChange(e.target.value)}
        >
          {availablePrograms.map(p => (
            <MenuItem key={p.code} value={p.code}>{p.name}</MenuItem>
          ))}
        </Select>
      </FormControl>

      <FormControl size="small" fullWidth>
        <InputLabel>Rok akademicki</InputLabel>
        <Select
          label="Rok akademicki"
          value={localConfig.year}
          onChange={e => handleYearChange(e.target.value as StudyYear)}
        >
          {availableYears.map(y => (
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
          {availableSemesters.map(n => (
            <MenuItem key={n} value={n}>
              {n} ({SEMESTER_TYPE[n] === 'Z' ? 'Zimowy' : 'Letni'})
            </MenuItem>
          ))}
        </Select>
      </FormControl>

      {/* Advanced: cookie — hidden in static build */}
      {!IS_STATIC && (
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
      )}

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
