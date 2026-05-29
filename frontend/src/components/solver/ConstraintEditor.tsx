'use client';

import { useState, useEffect } from 'react';
import {
  Dialog, DialogTitle, DialogContent, DialogActions,
  Button, FormControl, InputLabel, Select, MenuItem,
  TextField, Checkbox, FormControlLabel, Box, Grid,
  Typography, Divider,
} from '@mui/material';
import type { Constraint, ConstraintType } from '@/types';
import { useAppStore } from '@/store/appStore';

const CONSTRAINT_TYPES: { value: ConstraintType; label: string; description: string }[] = [
  { value: 'minimize_total_absence',  label: 'Minimalizuj łączne nieobecności',        description: 'Minimalizuje nieobecności na zajęciach.' },
  { value: 'minimize_gaps',           label: 'Minimalizuj okienka',                    description: 'Minimalizuje przerwy powyżej progu min_break_duration.' },
  { value: 'group_preference',        label: 'Preferencja grupy',                      description: 'Wymusza lub preferuje wybraną grupę dla zajęć.' },
  { value: 'lecturer_preference',     label: 'Preferencja prowadzącego',               description: 'Preferuje zajęcia z danym prowadzącym.' },
  { value: 'minimize_class_absence',  label: 'Minimalizuj nieobecności (zajęcia)',      description: 'Minimalizuje opuszczone sesje dla danych zajęć.' },
  { value: 'minimize_group_absence',  label: 'Minimalizuj nieobecności (grupa)',        description: 'Minimalizuje opuszczone sesje dla grupy zajęć.' },
  { value: 'time_block_day',          label: 'Blok godzinowy (dzień)',                 description: 'Zabrania/preferuje zajęcia w danym przedziale godzinowym.' },
  { value: 'time_block_date',         label: 'Blok godzinowy (data)',                  description: 'Zabrania/preferuje zajęcia konkretnego dnia.' },
  { value: 'prefer_edge_class',       label: 'Preferencja brzegowa (zajęcia)',         description: 'Preferuje zajęcia na początku/końcu dnia.' },
  { value: 'prefer_edge_group',       label: 'Preferencja brzegowa (grupa)',           description: 'Preferuje grupę na początku/końcu dnia.' },
];

const DAYS = ['Niedziela', 'Poniedziałek', 'Wtorek', 'Środa', 'Czwartek', 'Piątek', 'Sobota'];

const CLASS_HAS_TYPE_FIELDS: ConstraintType[] = [
  'group_preference', 'lecturer_preference',
  'minimize_class_absence', 'minimize_group_absence',
  'prefer_edge_class', 'prefer_edge_group',
];
const CLASS_HAS_GROUP: ConstraintType[] = [
  'group_preference', 'minimize_group_absence', 'prefer_edge_group',
];

function makeDefault(type: ConstraintType, seq: number): Constraint {
  const base: Constraint = { constraint_type: type, sequence: seq, weight: 1, hard: false, slack: 0 };
  switch (type) {
    case 'minimize_gaps':          return { ...base, min_break_duration: 0 };
    case 'group_preference':       return { ...base, class_id: '', class_type: '', group: 1 };
    case 'lecturer_preference':    return { ...base, class_id: '', class_type: '', lecturer: '' };
    case 'minimize_class_absence': return { ...base, class_id: '', class_type: '' };
    case 'minimize_group_absence': return { ...base, class_id: '', class_type: '', group: 1 };
    case 'time_block_day':         return { ...base, day: 1, start_time: 480, end_time: 570 };
    case 'time_block_date':        return { ...base, date: new Date().toISOString().slice(0, 10), start_time: 480, end_time: 570 };
    case 'prefer_edge_class':      return { ...base, class_id: '', class_type: '', position: 'start' };
    case 'prefer_edge_group':      return { ...base, class_id: '', class_type: '', group: 1, position: 'start' };
    default:                       return base;
  }
}

interface Props {
  open: boolean;
  initial?: Constraint;
  editIndex?: number;
  onClose: () => void;
}

export default function ConstraintEditor({ open, initial, editIndex, onClose }: Props) {
  const constraints      = useAppStore(s => s.constraints);
  const addConstraint    = useAppStore(s => s.addConstraint);
  const updateConstraint = useAppStore(s => s.updateConstraint);
  const courseGroups     = useAppStore(s => s.courseGroups);

  const nextSeq = Math.max(0, ...constraints.map(c => c.sequence)) + 1;

  const [draft, setDraft] = useState<Constraint>(
    initial ?? makeDefault('minimize_total_absence', nextSeq)
  );

  useEffect(() => {
    if (open) setDraft(initial ?? makeDefault('minimize_total_absence', nextSeq));
  }, [open]);

  function set<K extends keyof Constraint>(key: K, val: Constraint[K]) {
    setDraft(d => ({ ...d, [key]: val }));
  }

  function handleTypeChange(type: ConstraintType) {
    setDraft(makeDefault(type, draft.sequence));
  }

  function handleClassIdChange(classId: string) {
    const course = courseGroups.find(g => g.id === classId);
    const types = course ? [...new Set(course.classes.map(c => c.class_type))].sort() : [];
    const firstType = types[0] ?? '';
    const classesOfType = course?.classes.filter(c => c.class_type === firstType) ?? [];
    setDraft(d => ({
      ...d,
      class_id: classId,
      class_type: firstType,
      ...(d.constraint_type === 'lecturer_preference'
        ? { lecturer: classesOfType[0]?.lecturer ?? '' }
        : {}),
      ...(CLASS_HAS_GROUP.includes(d.constraint_type)
        ? { group: classesOfType[0]?.group ?? 1 }
        : {}),
    }));
  }

  function handleClassTypeChange(classType: string) {
    const course = courseGroups.find(g => g.id === (draft.class_id ?? ''));
    const classesOfType = course?.classes.filter(c => c.class_type === classType) ?? [];
    setDraft(d => ({
      ...d,
      class_type: classType,
      ...(d.constraint_type === 'lecturer_preference'
        ? { lecturer: classesOfType[0]?.lecturer ?? '' }
        : {}),
      ...(CLASS_HAS_GROUP.includes(d.constraint_type)
        ? { group: classesOfType[0]?.group ?? 1 }
        : {}),
    }));
  }

  function handleSave() {
    if (editIndex !== undefined) updateConstraint(editIndex, draft);
    else addConstraint(draft);
    onClose();
  }

  const def = CONSTRAINT_TYPES.find(t => t.value === draft.constraint_type);

  // Cascading options derived from selected course + type
  const selectedCourse = courseGroups.find(g => g.id === (draft.class_id ?? ''));
  const availableTypes = selectedCourse
    ? [...new Set(selectedCourse.classes.map(c => c.class_type))].sort()
    : [];
  const classesOfType = selectedCourse && draft.class_type
    ? selectedCourse.classes.filter(c => c.class_type === draft.class_type)
    : [];
  const availableLecturers = [...new Set(classesOfType.map(c => c.lecturer))].filter(Boolean).sort();
  const availableGroups    = [...new Set(classesOfType.map(c => c.group))].sort((a, b) => a - b);

  const num = (val: number | undefined, key: keyof Constraint, label: string) => (
    <TextField
      label={label}
      type="number"
      size="small"
      value={val ?? ''}
      onChange={e => set(key, Number(e.target.value) as never)}
      fullWidth
    />
  );

  return (
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle>
        {editIndex !== undefined ? 'Edytuj ograniczenie' : 'Dodaj ograniczenie'}
      </DialogTitle>
      <DialogContent>
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2, pt: 0.5 }}>

          {/* Constraint type */}
          <FormControl size="small" fullWidth>
            <InputLabel>Typ</InputLabel>
            <Select
              label="Typ"
              value={draft.constraint_type}
              onChange={e => handleTypeChange(e.target.value as ConstraintType)}
            >
              {CONSTRAINT_TYPES.map(t => (
                <MenuItem key={t.value} value={t.value}>{t.label}</MenuItem>
              ))}
            </Select>
          </FormControl>

          {def && (
            <Typography variant="caption" color="text.secondary">{def.description}</Typography>
          )}

          <Divider />

          {/* Common numeric fields */}
          <Grid container spacing={1.5}>
            <Grid item xs={4}>{num(draft.weight,   'weight',   'Waga')}</Grid>
            <Grid item xs={4}>{num(draft.slack,    'slack',    'Luz (slack)')}</Grid>
            <Grid item xs={4}>{num(draft.sequence, 'sequence', 'Sekwencja')}</Grid>
          </Grid>

          <FormControlLabel
            control={<Checkbox checked={!!draft.hard} onChange={e => set('hard', e.target.checked)} size="small" />}
            label="Twarde ograniczenie (hard)"
          />

          <Divider />

          {/* minimize_gaps */}
          {draft.constraint_type === 'minimize_gaps' && (
            num(draft.min_break_duration, 'min_break_duration', 'Min. przerwa (min)')
          )}

          {/* Fields for constraints that reference a class */}
          {CLASS_HAS_TYPE_FIELDS.includes(draft.constraint_type) && (
            <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1.5 }}>
              {/* Step 1: course name */}
              <FormControl size="small" fullWidth>
                <InputLabel>Przedmiot</InputLabel>
                <Select
                  label="Przedmiot"
                  value={draft.class_id ?? ''}
                  onChange={e => handleClassIdChange(e.target.value)}
                >
                  {courseGroups.map(g => (
                    <MenuItem key={g.id} value={g.id}>{g.name || g.id}</MenuItem>
                  ))}
                </Select>
              </FormControl>

              {/* Step 2: class type (filtered to available) */}
              <FormControl size="small" fullWidth>
                <InputLabel>Typ zajęć</InputLabel>
                <Select
                  label="Typ zajęć"
                  value={draft.class_type ?? ''}
                  onChange={e => handleClassTypeChange(e.target.value)}
                  disabled={!draft.class_id || availableTypes.length === 0}
                >
                  {availableTypes.map(t => <MenuItem key={t} value={t}>{t}</MenuItem>)}
                </Select>
              </FormControl>

              {/* Step 3a: lecturer (filtered) */}
              {draft.constraint_type === 'lecturer_preference' && (
                <FormControl size="small" fullWidth>
                  <InputLabel>Prowadzący</InputLabel>
                  <Select
                    label="Prowadzący"
                    value={draft.lecturer ?? ''}
                    onChange={e => set('lecturer', e.target.value)}
                    disabled={!draft.class_type || availableLecturers.length === 0}
                  >
                    {availableLecturers.map(l => <MenuItem key={l} value={l}>{l}</MenuItem>)}
                  </Select>
                </FormControl>
              )}

              {/* Step 3b: group (filtered) */}
              {CLASS_HAS_GROUP.includes(draft.constraint_type) && (
                <FormControl size="small" fullWidth>
                  <InputLabel>Nr grupy</InputLabel>
                  <Select
                    label="Nr grupy"
                    value={draft.group ?? ''}
                    onChange={e => set('group', Number(e.target.value))}
                    disabled={!draft.class_type || availableGroups.length === 0}
                  >
                    {availableGroups.map(g => <MenuItem key={g} value={g}>{g}</MenuItem>)}
                  </Select>
                </FormControl>
              )}
            </Box>
          )}

          {/* time_block_day */}
          {draft.constraint_type === 'time_block_day' && (
            <Grid container spacing={1.5}>
              <Grid item xs={4}>
                <FormControl size="small" fullWidth>
                  <InputLabel>Dzień</InputLabel>
                  <Select
                    label="Dzień"
                    value={draft.day ?? 1}
                    onChange={e => set('day', Number(e.target.value))}
                  >
                    {DAYS.map((d, i) => <MenuItem key={i} value={i}>{d}</MenuItem>)}
                  </Select>
                </FormControl>
              </Grid>
              <Grid item xs={4}>{num(draft.start_time, 'start_time', 'Od (min)')}</Grid>
              <Grid item xs={4}>{num(draft.end_time,   'end_time',   'Do (min)')}</Grid>
            </Grid>
          )}

          {/* time_block_date */}
          {draft.constraint_type === 'time_block_date' && (
            <Grid container spacing={1.5}>
              <Grid item xs={4}>
                <TextField
                  label="Data (YYYY-MM-DD)"
                  size="small"
                  type="date"
                  value={draft.date ?? ''}
                  onChange={e => set('date', e.target.value)}
                  fullWidth
                  InputLabelProps={{ shrink: true }}
                />
              </Grid>
              <Grid item xs={4}>{num(draft.start_time, 'start_time', 'Od (min)')}</Grid>
              <Grid item xs={4}>{num(draft.end_time,   'end_time',   'Do (min)')}</Grid>
            </Grid>
          )}

          {/* edge position */}
          {['prefer_edge_class', 'prefer_edge_group'].includes(draft.constraint_type) && (
            <FormControl size="small" fullWidth>
              <InputLabel>Pozycja</InputLabel>
              <Select
                label="Pozycja"
                value={draft.position ?? 'start'}
                onChange={e => set('position', e.target.value as 'start' | 'end')}
              >
                <MenuItem value="start">Początek dnia</MenuItem>
                <MenuItem value="end">Koniec dnia</MenuItem>
              </Select>
            </FormControl>
          )}

        </Box>
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>Anuluj</Button>
        <Button variant="contained" onClick={handleSave}>Zapisz</Button>
      </DialogActions>
    </Dialog>
  );
}