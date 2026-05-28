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
  { value: 'minimize_total_absence',  label: 'Minimalizuj łączne okienka',    description: 'Minimalizuje sumę przerw między zajęciami w ciągu dnia.' },
  { value: 'minimize_gaps',           label: 'Minimalizuj okienka',            description: 'Minimalizuje przerwy powyżej progu min_break_duration.' },
  { value: 'group_preference',        label: 'Preferencja grupy',              description: 'Wymusza lub preferuje wybraną grupę dla zajęć.' },
  { value: 'lecturer_preference',     label: 'Preferencja prowadzącego',       description: 'Preferuje zajęcia z danym prowadzącym.' },
  { value: 'minimize_class_absence',  label: 'Minimalizuj nieobecności (zajęcia)', description: 'Minimalizuje opuszczone sesje dla danych zajęć.' },
  { value: 'minimize_group_absence',  label: 'Minimalizuj nieobecności (grupa)',   description: 'Minimalizuje opuszczone sesje dla grupy zajęć.' },
  { value: 'time_block_day',          label: 'Blok godzinowy (dzień)',         description: 'Zabrania/preferuje zajęcia w danym przedziale godzinowym.' },
  { value: 'time_block_date',         label: 'Blok godzinowy (data)',          description: 'Zabrania/preferuje zajęcia konkretnego dnia.' },
  { value: 'prefer_edge_class',       label: 'Preferencja brzegowa (zajęcia)', description: 'Preferuje zajęcia na początku/końcu dnia.' },
  { value: 'prefer_edge_group',       label: 'Preferencja brzegowa (grupa)',   description: 'Preferuje grupę na początku/końcu dnia.' },
];

const DAYS = ['Niedziela','Poniedziałek','Wtorek','Środa','Czwartek','Piątek','Sobota'];

function makeDefault(type: ConstraintType, seq: number): Constraint {
  const base: Constraint = { constraint_type: type, sequence: seq, weight: 1, hard: false, slack: 0 };
  switch (type) {
    case 'minimize_gaps':       return { ...base, min_break_duration: 0 };
    case 'group_preference':    return { ...base, class_id: '', class_type: 'CWL', group: 1 };
    case 'lecturer_preference': return { ...base, class_id: '', class_type: 'W', lecturer: '' };
    case 'minimize_class_absence': return { ...base, class_id: '', class_type: 'CWL' };
    case 'minimize_group_absence':  return { ...base, class_id: '', class_type: 'CWL', group: 1 };
    case 'time_block_day':      return { ...base, day: 1, start_time: 480, end_time: 570 };
    case 'time_block_date':     return { ...base, date: new Date().toISOString().slice(0,10), start_time: 480, end_time: 570 };
    case 'prefer_edge_class':   return { ...base, class_id: '', class_type: 'W', position: 'start' };
    case 'prefer_edge_group':   return { ...base, class_id: '', class_type: 'W', group: 1, position: 'start' };
    default:                    return base;
  }
}

interface Props {
  open: boolean;
  initial?: Constraint;
  editIndex?: number;
  onClose: () => void;
}

export default function ConstraintEditor({ open, initial, editIndex, onClose }: Props) {
  const constraints = useAppStore(s => s.constraints);
  const addConstraint    = useAppStore(s => s.addConstraint);
  const updateConstraint = useAppStore(s => s.updateConstraint);
  const courseGroups     = useAppStore(s => s.courseGroups);

  const nextSeq = Math.max(0, ...constraints.map(c => c.sequence)) + 1;

  const [draft, setDraft] = useState<Constraint>(
    initial ?? makeDefault('minimize_total_absence', nextSeq)
  );

  useEffect(() => {
    if (open) {
      setDraft(initial ?? makeDefault('minimize_total_absence', nextSeq));
    }
  }, [open]);

  function set<K extends keyof Constraint>(key: K, val: Constraint[K]) {
    setDraft(d => ({ ...d, [key]: val }));
  }

  function handleTypeChange(type: ConstraintType) {
    setDraft(makeDefault(type, draft.sequence));
  }

  function handleSave() {
    if (editIndex !== undefined) {
      updateConstraint(editIndex, draft);
    } else {
      addConstraint(draft);
    }
    onClose();
  }

  const courseIds = courseGroups.map(g => g.id);
  const def = CONSTRAINT_TYPES.find(t => t.value === draft.constraint_type);

  const num = (val: number | undefined, key: keyof Constraint, label: string) => (
    <TextField
      label={label}
      type="number"
      size="small"
      value={val ?? ''}
      onChange={e => set(key, Number(e.target.value) as any)}
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

          <Grid container spacing={1.5}>
            <Grid item xs={4}>{num(draft.weight, 'weight', 'Waga')}</Grid>
            <Grid item xs={4}>{num(draft.slack, 'slack', 'Luz (slack)')}</Grid>
            <Grid item xs={4}>{num(draft.sequence, 'sequence', 'Sekwencja')}</Grid>
          </Grid>

          <FormControlLabel
            control={<Checkbox checked={!!draft.hard} onChange={e => set('hard', e.target.checked)} size="small" />}
            label="Twarde ograniczenie (hard)"
          />

          <Divider />

          {/* Type-specific fields */}
          {draft.constraint_type === 'minimize_gaps' && (
            num(draft.min_break_duration, 'min_break_duration', 'Min. przerwa (min)')
          )}

          {['group_preference','lecturer_preference','minimize_class_absence','minimize_group_absence','prefer_edge_class','prefer_edge_group'].includes(draft.constraint_type) && (
            <Grid container spacing={1.5}>
              <Grid item xs={6}>
                <FormControl size="small" fullWidth>
                  <InputLabel>Przedmiot (ID)</InputLabel>
                  <Select
                    label="Przedmiot (ID)"
                    value={draft.class_id ?? ''}
                    onChange={e => set('class_id', e.target.value)}
                  >
                    {courseIds.map(id => <MenuItem key={id} value={id}>{id}</MenuItem>)}
                  </Select>
                </FormControl>
              </Grid>
              <Grid item xs={6}>
                <TextField
                  label="Typ zajęć (W/CWL/…)"
                  size="small"
                  value={draft.class_type ?? ''}
                  onChange={e => set('class_type', e.target.value)}
                  fullWidth
                />
              </Grid>
            </Grid>
          )}

          {['group_preference','minimize_group_absence','prefer_edge_group'].includes(draft.constraint_type) && (
            num(draft.group, 'group', 'Nr grupy')
          )}

          {draft.constraint_type === 'lecturer_preference' && (
            <TextField
              label="Prowadzący"
              size="small"
              value={draft.lecturer ?? ''}
              onChange={e => set('lecturer', e.target.value)}
              fullWidth
            />
          )}

          {draft.constraint_type === 'time_block_day' && (
            <Grid container spacing={1.5}>
              <Grid item xs={4}>
                <FormControl size="small" fullWidth>
                  <InputLabel>Dzień</InputLabel>
                  <Select label="Dzień" value={draft.day ?? 1} onChange={e => set('day', Number(e.target.value))}>
                    {DAYS.map((d, i) => <MenuItem key={i} value={i}>{d}</MenuItem>)}
                  </Select>
                </FormControl>
              </Grid>
              <Grid item xs={4}>{num(draft.start_time, 'start_time', 'Od (min)')}</Grid>
              <Grid item xs={4}>{num(draft.end_time, 'end_time', 'Do (min)')}</Grid>
            </Grid>
          )}

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
              <Grid item xs={4}>{num(draft.end_time, 'end_time', 'Do (min)')}</Grid>
            </Grid>
          )}

          {['prefer_edge_class','prefer_edge_group'].includes(draft.constraint_type) && (
            <FormControl size="small" fullWidth>
              <InputLabel>Pozycja</InputLabel>
              <Select label="Pozycja" value={draft.position ?? 'start'} onChange={e => set('position', e.target.value as 'start'|'end')}>
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
