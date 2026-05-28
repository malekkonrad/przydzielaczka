'use client';

import { useMemo, useState, useCallback } from 'react';
import { Calendar, dateFnsLocalizer, View, type Event } from 'react-big-calendar';
import format from 'date-fns/format';
import parse from 'date-fns/parse';
import startOfWeek from 'date-fns/startOfWeek';
import getDay from 'date-fns/getDay';
import addMinutes from 'date-fns/addMinutes';
import setHours from 'date-fns/setHours';
import setMinutes from 'date-fns/setMinutes';
import setDay from 'date-fns/setDay';
import { pl } from 'date-fns/locale';
import {
  Box, ToggleButtonGroup, ToggleButton, Tooltip, Typography,
  useTheme,
} from '@mui/material';
import { useAppStore } from '@/store/appStore';
import type { CalendarEvent, CourseClass, SolverRun } from '@/types';
import { COURSE_COLORS } from '@/lib/usos/constants';

// ─── localizer ───────────────────────────────────────────────────────────────

const localizer = dateFnsLocalizer({
  format,
  parse,
  startOfWeek: () => startOfWeek(new Date(), { weekStartsOn: 1 }),
  getDay,
  locales: { pl },
});

// ─── helpers ─────────────────────────────────────────────────────────────────

const PATTERN_MONDAY = new Date(2026, 2, 2); // 2026-03-02 as reference week

function minutesToDate(base: Date, minutes: number): Date {
  const h = Math.floor(minutes / 60);
  const m = minutes % 60;
  return setMinutes(setHours(base, h), m);
}

// Synthesize a "pattern week" event (no specific date, just day + time)
function classToPatternEvent(cls: CourseClass, color: string, source: 'original' | 'solver'): CalendarEvent[] {
  // Day 1=Mon → setDay(..., 1, {weekStartsOn:1})
  const dayBase = setDay(PATTERN_MONDAY, cls.day, { weekStartsOn: 1 });
  const start   = minutesToDate(dayBase, cls.start_time);
  const end     = minutesToDate(dayBase, cls.end_time);

  return [{
    id:    `${source}-${cls.id}-${cls.class_type}-${cls.group}-pattern`,
    title: `${cls.class_type} gr.${cls.group}`,
    start,
    end,
    resource: {
      courseId: cls.id,
      courseName: cls.name ?? cls.id,
      classType: cls.class_type,
      group: cls.group,
      lecturer: cls.lecturer,
      room: cls.location.room,
      building: cls.location.building,
      week: cls.week,
      source,
      color,
    },
  }];
}

function classToDateEvents(cls: CourseClass, color: string, source: 'original' | 'solver'): CalendarEvent[] {
  if (!cls.sessions?.length) return classToPatternEvent(cls, color, source);

  return cls.sessions.map((s, i) => {
    const base  = parse(s.date, 'yyyy-MM-dd', new Date());
    const start = minutesToDate(base, s.start_time);
    const end   = minutesToDate(base, s.end_time);
    return {
      id:    `${source}-${cls.id}-${cls.class_type}-${cls.group}-${s.date}-${i}`,
      title: `${cls.class_type} gr.${cls.group}`,
      start,
      end,
      resource: {
        courseId: cls.id,
        courseName: cls.name ?? cls.id,
        classType: cls.class_type,
        group: cls.group,
        lecturer: cls.lecturer,
        room: s.location.room || cls.location.room,
        building: s.location.building || cls.location.building,
        week: cls.week,
        source,
        color,
      },
    };
  });
}

// ─── event component ─────────────────────────────────────────────────────────

function EventComponent({ event }: { event: CalendarEvent }) {
  const r = event.resource;
  return (
    <Tooltip
      title={
        <Box sx={{ fontSize: '0.75rem', lineHeight: 1.5 }}>
          <b>{r.courseName}</b><br />
          {r.classType} · gr.{r.group} · {r.week}<br />
          {r.lecturer}<br />
          sala {r.room}, {r.building}
          {r.source === 'solver' && <><br /><em>← wynik solvera</em></>}
        </Box>
      }
      placement="top"
      arrow
    >
      <Box sx={{ height: '100%', overflow: 'hidden', lineHeight: 1.2 }}>
        <Typography sx={{ fontSize: '0.68rem', fontWeight: 600, lineHeight: 1.2 }}>
          {event.title}
        </Typography>
        <Typography sx={{ fontSize: '0.6rem', opacity: 0.85 }}>
          {r.room} · {r.building}
        </Typography>
        {r.week !== 'AB' && (
          <Typography sx={{ fontSize: '0.58rem', opacity: 0.7 }}>
            tyg. {r.week}
          </Typography>
        )}
      </Box>
    </Tooltip>
  );
}

// ─── main calendar ────────────────────────────────────────────────────────────

export default function TimetableCalendar() {
  const theme = useTheme();

  const courseGroups       = useAppStore(s => s.courseGroups);
  const selectedCourseIds  = useAppStore(s => s.selectedCourseIds);
  const disabledGroupKeys  = useAppStore(s => s.disabledGroupKeys);
  const colorMap           = useAppStore(s => s.courseColorMap);
  const calendarMode       = useAppStore(s => s.calendarMode);
  const setCalendarMode    = useAppStore(s => s.setCalendarMode);
  const showOriginalPlan   = useAppStore(s => s.showOriginalPlan);
  const showAllGroups      = useAppStore(s => s.showAllGroups);
  const setShowAllGroups   = useAppStore(s => s.setShowAllGroups);
  const solverRuns         = useAppStore(s => s.solverRuns);
  const activeSolverRunId  = useAppStore(s => s.activeSolverRunId);
  const activeSolIdxMap    = useAppStore(s => s.activeSolutionIndex);

  const [view, setView] = useState<View>('week');
  const [date, setDate] = useState<Date>(PATTERN_MONDAY);

  // Collect events
  const events = useMemo<CalendarEvent[]>(() => {
    const result: CalendarEvent[] = [];

    // Original plan events (gated by showOriginalPlan + showAllGroups)
    if (showOriginalPlan && showAllGroups) {
      for (const group of courseGroups) {
        if (!selectedCourseIds.has(group.id)) continue;
        const color = colorMap[group.id] ?? '#1976d2';

        for (const cls of group.classes) {
          const key = `${group.id}::${cls.class_type}::${cls.group}`;
          if (disabledGroupKeys.has(key)) continue;
          const evts = calendarMode === 'pattern'
            ? classToPatternEvent(cls, color, 'original')
            : classToDateEvents(cls, color, 'original');
          result.push(...evts);
        }
      }
    }

    // Solver result events
    if (activeSolverRunId) {
      const run = solverRuns.find(r => r.id === activeSolverRunId);
      if (run) {
        const solIdx = activeSolIdxMap[activeSolverRunId] ?? 0;
        const solResult = run.results[solIdx];
        if (solResult) {
          const assignments = Array.isArray(solResult.solutions)
            ? solResult.solutions
            : [];

          for (const assign of assignments) {
            const group = courseGroups.find(g => g.id === assign.class_id);
            if (!group) continue;
            const cls = group.classes.find(
              c => c.class_type === assign.class_type && c.group === assign.group
            );
            if (!cls) continue;
            const color = colorMap[group.id] ?? '#9c27b0';
            const evts = calendarMode === 'pattern'
              ? classToPatternEvent(cls, color, 'solver')
              : classToDateEvents(cls, color, 'solver');
            result.push(...evts);
          }
        }
      }
    }

    return result;
  }, [courseGroups, selectedCourseIds, disabledGroupKeys, colorMap, calendarMode, showOriginalPlan, showAllGroups, solverRuns, activeSolverRunId, activeSolIdxMap]);

  const eventStyleGetter = useCallback((event: CalendarEvent) => {
    const r = event.resource;
    const isSolver = r.source === 'solver';
    return {
      style: {
        backgroundColor: r.color,
        opacity: isSolver ? 1 : 0.85,
        border: isSolver ? `2px solid white` : 'none',
        borderRadius: 4,
        color: '#fff',
      },
    };
  }, []);

  const calBg   = theme.palette.background.paper;
  const border  = theme.palette.divider;
  const textPri = theme.palette.text.primary;
  const textSec = theme.palette.text.secondary;

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%', p: 1, gap: 1 }}>
      {/* Toolbar */}
      <Box sx={{ display: 'flex', gap: 1, alignItems: 'center', flexWrap: 'wrap' }}>
        <ToggleButtonGroup
          size="small"
          exclusive
          value={calendarMode}
          onChange={(_, v) => v && setCalendarMode(v)}
        >
          <ToggleButton value="pattern">Wzorzec</ToggleButton>
          <ToggleButton value="dates">Terminy</ToggleButton>
        </ToggleButtonGroup>

        <ToggleButtonGroup
          size="small"
          exclusive
          value={view}
          onChange={(_, v) => v && setView(v)}
        >
          <ToggleButton value="week">Tydzień</ToggleButton>
          <ToggleButton value="month">Miesiąc</ToggleButton>
          <ToggleButton value="day">Dzień</ToggleButton>
        </ToggleButtonGroup>

        <ToggleButtonGroup
          size="small"
          exclusive
          value={showAllGroups ? 'all' : 'selected'}
          onChange={(_, v) => v && setShowAllGroups(v === 'all')}
        >
          <ToggleButton value="all">Wszystkie grupy</ToggleButton>
          <ToggleButton value="selected">Wybrane</ToggleButton>
        </ToggleButtonGroup>

        {activeSolverRunId && (
          <Typography variant="caption" color="secondary" fontWeight={600}>
            ● Wynik solvera widoczny
          </Typography>
        )}
      </Box>

      {/* Calendar */}
      <Box sx={{
        flex: 1,
        minHeight: 0,
        '& .rbc-calendar': { color: textPri },
        '& .rbc-header': { borderColor: border, color: textPri },
        '& .rbc-time-view': { borderColor: border },
        '& .rbc-timeslot-group': { borderColor: border },
        '& .rbc-day-slot .rbc-time-slot': { borderColor: 'transparent' },
        '& .rbc-off-range-bg': { bgcolor: theme.palette.action.hover },
        '& .rbc-label, .rbc-time-slot': { color: textSec, fontSize: '0.7rem' },
      }}>
        <Calendar
          localizer={localizer}
          events={events as Event[]}
          view={view}
          date={date}
          onView={setView}
          onNavigate={setDate}
          startAccessor="start"
          endAccessor="end"
          titleAccessor="title"
          style={{ height: '100%', background: calBg }}
          components={{
            event: EventComponent as any,
          }}
          eventPropGetter={eventStyleGetter as any}
          min={setMinutes(setHours(new Date(), 7), 0)}
          max={setMinutes(setHours(new Date(), 21), 0)}
          step={15}
          timeslots={4}
          culture="pl"
          messages={{
            today: 'Dziś',
            previous: '‹',
            next: '›',
            week: 'Tydzień',
            month: 'Miesiąc',
            day: 'Dzień',
            showMore: (n) => `+${n} więcej`,
          }}
        />
      </Box>
    </Box>
  );
}
