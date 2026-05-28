'use client';

import { useState } from 'react';
import {
  Box, Typography, Checkbox, Chip, Tooltip, IconButton,
  Divider, Stack, Collapse,
} from '@mui/material';
import {
  SelectAll, DoneAll, Delete, DeleteForever,
  ExpandMore, ExpandLess, Visibility, VisibilityOff,
} from '@mui/icons-material';
import { useAppStore, groupKey } from '@/store/appStore';
import { CLASS_TYPE_LABELS } from '@/lib/usos/constants';

function CourseRow({ courseId }: { courseId: string }) {
  const [open, setOpen] = useState(false);

  const group           = useAppStore(s => s.courseGroups.find(g => g.id === courseId))!;
  const selected        = useAppStore(s => s.selectedCourseIds.has(courseId));
  const disabledKeys    = useAppStore(s => s.disabledGroupKeys);
  const color           = useAppStore(s => s.courseColorMap[courseId] ?? '#1976d2');
  const toggleCourse    = useAppStore(s => s.toggleCourseSelection);
  const toggleGroupKey  = useAppStore(s => s.toggleGroupKey);
  const removeCourse    = useAppStore(s => s.removeCourse);

  // Unique (class_type, group) pairs, sorted
  const classPairs = Array.from(
    new Map(
      group.classes.map(c => [`${c.class_type}::${c.group}`, { class_type: c.class_type, group: c.group }])
    ).values()
  ).sort((a, b) => a.class_type.localeCompare(b.class_type) || a.group - b.group);

  return (
    <Box>
      <Box
        sx={{
          display: 'flex',
          alignItems: 'flex-start',
          px: 1,
          py: 0.5,
          gap: 0.5,
          '&:hover': { bgcolor: 'action.hover' },
        }}
      >
        {/* expand toggle */}
        <IconButton
          size="small"
          sx={{ p: 0.25, mt: '2px', flexShrink: 0 }}
          onClick={() => setOpen(v => !v)}
        >
          {open ? <ExpandLess fontSize="small" /> : <ExpandMore fontSize="small" />}
        </IconButton>

        {/* colour swatch + name — clicking toggles course selection */}
        <Box
          sx={{ flex: 1, minWidth: 0, cursor: 'pointer' }}
          onClick={() => toggleCourse(courseId)}
        >
          <Stack direction="row" alignItems="center" spacing={0.5}>
            <Box
              sx={{
                width: 10,
                height: 10,
                borderRadius: '2px',
                bgcolor: selected ? color : 'transparent',
                border: `2px solid ${color}`,
                flexShrink: 0,
              }}
            />
            <Typography
              variant="body2"
              sx={{
                fontWeight: 500,
                fontSize: '0.78rem',
                lineHeight: 1.3,
                color: selected ? 'text.primary' : 'text.disabled',
                wordBreak: 'break-word',
              }}
            >
              {group.name}
            </Typography>
          </Stack>
          <Typography variant="caption" color="text.secondary" sx={{ fontSize: '0.68rem', pl: 2 }}>
            {group.id}
          </Typography>
        </Box>

        {/* delete */}
        <Tooltip title="Usuń przedmiot">
          <IconButton
            size="small"
            sx={{ p: 0.25, mt: '2px', flexShrink: 0, color: 'text.disabled', '&:hover': { color: 'error.main' } }}
            onClick={e => { e.stopPropagation(); removeCourse(courseId); }}
          >
            <Delete fontSize="small" />
          </IconButton>
        </Tooltip>

        {/* course checkbox */}
        <Checkbox
          size="small"
          checked={selected}
          onChange={() => toggleCourse(courseId)}
          sx={{ p: 0.25, color }}
        />
      </Box>

      {/* expanded group list */}
      <Collapse in={open}>
        <Box sx={{ pl: 4, pr: 1, pb: 0.5 }}>
          {classPairs.map(({ class_type, group: grp }) => {
            const key     = groupKey(courseId, class_type, grp);
            const enabled = !disabledKeys.has(key);
            return (
              <Box
                key={key}
                sx={{
                  display: 'flex',
                  alignItems: 'center',
                  gap: 0.5,
                  py: 0.2,
                  cursor: 'pointer',
                  opacity: enabled ? 1 : 0.4,
                }}
                onClick={() => toggleGroupKey(key)}
              >
                <Checkbox
                  size="small"
                  checked={enabled}
                  onChange={() => toggleGroupKey(key)}
                  onClick={e => e.stopPropagation()}
                  sx={{ p: 0, color }}
                />
                <Chip
                  label={CLASS_TYPE_LABELS[class_type] ?? class_type}
                  size="small"
                  sx={{ height: 16, fontSize: '0.6rem', '& .MuiChip-label': { px: 0.5 } }}
                />
                <Typography variant="caption" sx={{ fontSize: '0.7rem' }}>
                  gr. {grp}
                </Typography>
              </Box>
            );
          })}
        </Box>
      </Collapse>

      <Divider />
    </Box>
  );
}

export default function CourseList() {
  const courseGroups      = useAppStore(s => s.courseGroups);
  const selectAll         = useAppStore(s => s.selectAll);
  const deselectAll       = useAppStore(s => s.deselectAll);
  const removeAllCourses  = useAppStore(s => s.removeAllCourses);
  const showOriginalPlan  = useAppStore(s => s.showOriginalPlan);
  const setShowOriginalPlan = useAppStore(s => s.setShowOriginalPlan);

  if (courseGroups.length === 0) {
    return (
      <Box sx={{ p: 2, textAlign: 'center' }}>
        <Typography variant="caption" color="text.secondary">
          Pobierz plan, aby zobaczyć przedmioty.
        </Typography>
      </Box>
    );
  }

  return (
    <Box>
      <Box sx={{ px: 1.5, pt: 1.5, pb: 0.5 }}>
        <Stack direction="row" alignItems="center" justifyContent="space-between">
          <Typography variant="caption" color="text.secondary" fontWeight={600} letterSpacing={0.5}>
            PRZEDMIOTY ({courseGroups.length})
          </Typography>
          <Box sx={{ display: 'flex', gap: 0 }}>
            <Tooltip title={showOriginalPlan ? 'Ukryj plan zajęć' : 'Pokaż plan zajęć'}>
              <IconButton size="small" onClick={() => setShowOriginalPlan(!showOriginalPlan)}>
                {showOriginalPlan
                  ? <Visibility fontSize="small" color="primary" />
                  : <VisibilityOff fontSize="small" />}
              </IconButton>
            </Tooltip>
            <Tooltip title="Zaznacz wszystkie">
              <IconButton size="small" onClick={selectAll}><SelectAll fontSize="small" /></IconButton>
            </Tooltip>
            <Tooltip title="Odznacz wszystkie">
              <IconButton size="small" onClick={deselectAll}><DoneAll fontSize="small" sx={{ opacity: 0.4 }} /></IconButton>
            </Tooltip>
            <Tooltip title="Usuń wszystkie przedmioty">
              <IconButton size="small" onClick={removeAllCourses} sx={{ color: 'text.disabled', '&:hover': { color: 'error.main' } }}>
                <DeleteForever fontSize="small" />
              </IconButton>
            </Tooltip>
          </Box>
        </Stack>
      </Box>

      <Divider />

      {courseGroups.map(g => (
        <CourseRow key={g.id} courseId={g.id} />
      ))}
    </Box>
  );
}