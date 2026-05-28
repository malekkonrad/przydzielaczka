import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type {
  CourseGroup,
  CourseClass,
  Constraint,
  SolverRun,
  StudyConfig,
  CalendarMode,
} from '@/types';
import { COURSE_COLORS } from '@/lib/usos/constants';

// ─── state ────────────────────────────────────────────────────────────────────

export function groupKey(courseId: string, classType: string, group: number) {
  return `${courseId}::${classType}::${group}`;
}

interface AppState {
  // Theme
  darkMode: boolean;
  toggleDarkMode: () => void;

  // Study selection
  studyConfig: StudyConfig;
  setStudyConfig: (cfg: Partial<StudyConfig>) => void;

  // Loaded courses
  courseGroups: CourseGroup[];
  loadingCourses: boolean;
  coursesError: string | null;
  setCourseGroups: (groups: CourseGroup[]) => void;
  setLoadingCourses: (v: boolean) => void;
  setCoursesError: (e: string | null) => void;
  removeCourse: (id: string) => void;
  removeAllCourses: () => void;

  // Per-course colour mapping
  courseColorMap: Record<string, string>;

  // Selection (which course-IDs are "active")
  selectedCourseIds: Set<string>;
  toggleCourseSelection: (id: string) => void;
  selectAll: () => void;
  deselectAll: () => void;

  // Per-group visibility (disabled keys won't show in calendar or go to solver)
  disabledGroupKeys: Set<string>;
  toggleGroupKey: (key: string) => void;
  enableAllGroupsForCourse: (courseId: string) => void;

  // Calendar display
  calendarMode: CalendarMode;
  setCalendarMode: (m: CalendarMode) => void;
  showOriginalPlan: boolean;
  setShowOriginalPlan: (v: boolean) => void;
  showAllGroups: boolean;
  setShowAllGroups: (v: boolean) => void;
  activeSolverRunId: string | null;
  setActiveSolverRunId: (id: string | null) => void;

  // Constraints
  constraints: Constraint[];
  addConstraint: (c: Constraint) => void;
  updateConstraint: (idx: number, c: Constraint) => void;
  removeConstraint: (idx: number) => void;

  // Solver
  solverRunning: boolean;
  setSolverRunning: (v: boolean) => void;
  solverRuns: SolverRun[];
  addSolverRun: (run: SolverRun) => void;
  activeSolutionIndex: Record<string, number>;   // runId → solution index
  setActiveSolutionIndex: (runId: string, idx: number) => void;

  // Derived helpers
  getSelectedClasses: () => CourseClass[];
  getColorForCourse: (courseId: string) => string;
}

// ─── store ────────────────────────────────────────────────────────────────────

export const useAppStore = create<AppState>()(
  persist(
    (set, get) => ({
      // Theme
      darkMode: false,
      toggleDarkMode: () => set(s => ({ darkMode: !s.darkMode })),

      // Study selection
      studyConfig: { program: 'ISI', year: '25/26', semesterNumber: 6 },
      setStudyConfig: cfg =>
        set(s => ({ studyConfig: { ...s.studyConfig, ...cfg } })),

      // Courses
      courseGroups: [],
      loadingCourses: false,
      coursesError: null,
      courseColorMap: {},
      setCourseGroups: (groups) => {
        const colorMap: Record<string, string> = {};
        groups.forEach((g, i) => {
          colorMap[g.id] = COURSE_COLORS[i % COURSE_COLORS.length];
        });
        set({
          courseGroups: groups,
          courseColorMap: colorMap,
          selectedCourseIds: new Set(groups.map(g => g.id)),
          disabledGroupKeys: new Set(),
        });
      },
      setLoadingCourses: v => set({ loadingCourses: v }),
      setCoursesError: e => set({ coursesError: e }),
      removeCourse: (id) =>
        set(s => {
          const next = s.courseGroups.filter(g => g.id !== id);
          const colorMap: Record<string, string> = {};
          next.forEach((g, i) => { colorMap[g.id] = COURSE_COLORS[i % COURSE_COLORS.length]; });
          const sel = new Set(s.selectedCourseIds);
          sel.delete(id);
          const dis = new Set(s.disabledGroupKeys);
          for (const k of dis) { if (k.startsWith(`${id}::`)) dis.delete(k); }
          return { courseGroups: next, courseColorMap: colorMap, selectedCourseIds: sel, disabledGroupKeys: dis };
        }),
      removeAllCourses: () =>
        set({ courseGroups: [], courseColorMap: {}, selectedCourseIds: new Set(), disabledGroupKeys: new Set() }),

      // Selection
      selectedCourseIds: new Set(),
      toggleCourseSelection: id =>
        set(s => {
          const next = new Set(s.selectedCourseIds);
          next.has(id) ? next.delete(id) : next.add(id);
          return { selectedCourseIds: next };
        }),
      selectAll: () =>
        set(s => ({
          selectedCourseIds: new Set(s.courseGroups.map(g => g.id)),
        })),
      deselectAll: () => set({ selectedCourseIds: new Set() }),

      // Group-level visibility
      disabledGroupKeys: new Set(),
      toggleGroupKey: (key) =>
        set(s => {
          const next = new Set(s.disabledGroupKeys);
          next.has(key) ? next.delete(key) : next.add(key);
          return { disabledGroupKeys: next };
        }),
      enableAllGroupsForCourse: (courseId) =>
        set(s => {
          const next = new Set(s.disabledGroupKeys);
          for (const k of next) { if (k.startsWith(`${courseId}::`)) next.delete(k); }
          return { disabledGroupKeys: next };
        }),

      // Calendar display
      calendarMode: 'pattern',
      setCalendarMode: m => set({ calendarMode: m }),
      showOriginalPlan: true,
      setShowOriginalPlan: v => set({ showOriginalPlan: v }),
      showAllGroups: true,
      setShowAllGroups: v => set({ showAllGroups: v }),
      activeSolverRunId: null,
      setActiveSolverRunId: id => set({ activeSolverRunId: id }),

      // Constraints
      constraints: [
        { constraint_type: 'minimize_total_absence', sequence: 1, weight: 1, hard: false, slack: 0 },
        { constraint_type: 'minimize_gaps', sequence: 2, weight: 1, hard: false, slack: 0, min_break_duration: 0 },
      ],
      addConstraint: c => set(s => ({ constraints: [...s.constraints, c] })),
      updateConstraint: (idx, c) =>
        set(s => {
          const next = [...s.constraints];
          next[idx] = c;
          return { constraints: next };
        }),
      removeConstraint: idx =>
        set(s => ({ constraints: s.constraints.filter((_, i) => i !== idx) })),

      // Solver
      solverRunning: false,
      setSolverRunning: v => set({ solverRunning: v }),
      solverRuns: [],
      addSolverRun: run =>
        set(s => ({ solverRuns: [run, ...s.solverRuns] })),
      activeSolutionIndex: {},
      setActiveSolutionIndex: (runId, idx) =>
        set(s => ({
          activeSolutionIndex: { ...s.activeSolutionIndex, [runId]: idx },
        })),

      // Derived
      getSelectedClasses: () => {
        const { courseGroups, selectedCourseIds, disabledGroupKeys } = get();
        return courseGroups
          .filter(g => selectedCourseIds.has(g.id))
          .flatMap(g =>
            g.classes.filter(c => !disabledGroupKeys.has(groupKey(g.id, c.class_type, c.group)))
          );
      },
      getColorForCourse: (courseId) => {
        const { courseColorMap } = get();
        return courseColorMap[courseId] ?? '#1976d2';
      },
    }),
    {
      name: 'przydzielaczka-store',
      partialize: (s) => ({
        darkMode: s.darkMode,
        studyConfig: s.studyConfig,
        constraints: s.constraints,
      }),
    }
  )
);