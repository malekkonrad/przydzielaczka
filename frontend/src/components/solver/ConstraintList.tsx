'use client';

import { useState } from 'react';
import {
  Box, Typography, IconButton, Chip, Tooltip, Button,
  List, ListItem, ListItemText, ListItemSecondaryAction,
  Divider,
} from '@mui/material';
import { Add, Edit, Delete } from '@mui/icons-material';
import { useAppStore } from '@/store/appStore';
import type { Constraint } from '@/types';
import ConstraintEditor from './ConstraintEditor';

const TYPE_SHORT: Record<string, string> = {
  minimize_total_absence:  'brak okienek',
  minimize_gaps:           'okienka',
  group_preference:        'gr. prefer.',
  lecturer_preference:     'prow. prefer.',
  minimize_class_absence:  'nieob. (kurs)',
  minimize_group_absence:  'nieob. (gr)',
  time_block_day:          'blok-dzień',
  time_block_date:         'blok-data',
  prefer_edge_class:       'brzeg (kurs)',
  prefer_edge_group:       'brzeg (gr)',
};

export default function ConstraintList() {
  const constraints     = useAppStore(s => s.constraints);
  const removeConstraint = useAppStore(s => s.removeConstraint);

  const [dialogOpen, setDialogOpen]   = useState(false);
  const [editIdx,    setEditIdx]      = useState<number | undefined>(undefined);
  const [editData,   setEditData]     = useState<Constraint | undefined>(undefined);

  function openAdd() {
    setEditIdx(undefined);
    setEditData(undefined);
    setDialogOpen(true);
  }

  function openEdit(idx: number) {
    setEditIdx(idx);
    setEditData(constraints[idx]);
    setDialogOpen(true);
  }

  return (
    <Box>
      <Box sx={{ px: 1.5, pt: 1, pb: 0.5, display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
        <Typography variant="caption" color="text.secondary" fontWeight={600} letterSpacing={0.5}>
          OGRANICZENIA ({constraints.length})
        </Typography>
        <Button size="small" startIcon={<Add />} onClick={openAdd} sx={{ minWidth: 0, fontSize: '0.72rem' }}>
          Dodaj
        </Button>
      </Box>

      <Divider />

      {constraints.length === 0 && (
        <Typography variant="caption" color="text.secondary" sx={{ px: 2, py: 1, display: 'block' }}>
          Brak ograniczeń.
        </Typography>
      )}

      <List dense disablePadding>
        {constraints.map((c, i) => (
          <Box key={i}>
            <ListItem
              sx={{ pr: 8, '&:hover': { bgcolor: 'action.hover' } }}
              secondaryAction={
                <Box>
                  <Tooltip title="Edytuj"><IconButton size="small" onClick={() => openEdit(i)}><Edit fontSize="small" /></IconButton></Tooltip>
                  <Tooltip title="Usuń"><IconButton size="small" onClick={() => removeConstraint(i)} color="error"><Delete fontSize="small" /></IconButton></Tooltip>
                </Box>
              }
            >
              <ListItemText
                primary={
                  <Box sx={{ display: 'flex', alignItems: 'center', gap: 0.5, flexWrap: 'wrap' }}>
                    <Typography variant="body2" sx={{ fontSize: '0.75rem', fontWeight: 500 }}>
                      {TYPE_SHORT[c.constraint_type] ?? c.constraint_type}
                    </Typography>
                    {c.hard && <Chip label="hard" size="small" color="error" sx={{ height: 14, fontSize: '0.6rem' }} />}
                  </Box>
                }
                secondary={
                  <Typography variant="caption" color="text.secondary" sx={{ fontSize: '0.65rem' }}>
                    seq:{c.sequence} · w:{c.weight ?? 1} · slack:{c.slack ?? 0}
                    {c.class_id ? ` · ${c.class_id}` : ''}
                  </Typography>
                }
              />
            </ListItem>
            <Divider />
          </Box>
        ))}
      </List>

      <ConstraintEditor
        open={dialogOpen}
        initial={editData}
        editIndex={editIdx}
        onClose={() => setDialogOpen(false)}
      />
    </Box>
  );
}
