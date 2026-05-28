// ─── Solver input types (matching input.schema.json) ─────────────────────────

export interface Location {
  room: string;
  building: string;
}

export interface Session {
  date: string;        // "YYYY-MM-DD"
  location: Location;
  start_time: number;  // minutes from midnight
  end_time: number;
}

export interface CourseClass {
  id: string;          // course code, e.g. "120-ISI-1S-708"
  lecturer: string;
  day: number;         // 0=Sunday … 6=Saturday (USOS: 1=Monday)
  week: 'A' | 'B' | 'AB';
  location: Location;
  group: number;
  class_type: string;  // CWL, W, CWP, …
  start_time: number;
  end_time: number;
  sessions: Session[];
  // UI-only (not sent to solver)
  name?: string;
  zajCykId?: string;
}

export type ConstraintType =
  | 'minimize_gaps'
  | 'group_preference'
  | 'lecturer_preference'
  | 'minimize_class_absence'
  | 'minimize_group_absence'
  | 'minimize_total_absence'
  | 'time_block_day'
  | 'time_block_date'
  | 'prefer_edge_class'
  | 'prefer_edge_group';

export interface Constraint {
  constraint_type: ConstraintType;
  sequence: number;
  weight?: number;
  hard?: boolean;
  slack?: number;
  // Conditional fields
  class_id?: string;
  class_type?: string;
  min_break_duration?: number;
  group?: number;
  lecturer?: string;
  day?: number;
  date?: string;
  start_time?: number;
  end_time?: number;
  position?: 'start' | 'end';
}

export interface SolverInput {
  constraints: Constraint[];
  classes: CourseClass[];
}

// ─── Solver output types ──────────────────────────────────────────────────────

export interface SolutionAssignment {
  class_id: string;
  class_type: string;
  group: number | null;
}

export interface SolutionMeta {
  solver_version: string;
  solved_at: string;
  duration_ms: number;
  algorithm: string;
  input_classes_total: number;
  input_constraints_total: number;
}

export interface SolverResult {
  solutions: SolutionAssignment[];
  meta: SolutionMeta;
}

// ─── UI-level types ───────────────────────────────────────────────────────────

export interface CourseGroup {
  id: string;      // course code
  name: string;    // human-readable name
  classes: CourseClass[];
}

export type SemesterType = 'Z' | 'L';
export type StudyYear = '23/24' | '24/25' | '25/26' | '26/27';

export interface StudyConfig {
  program: string;        // e.g. "ISI"
  year: StudyYear;        // e.g. "25/26"
  semesterNumber: number; // 1–7
}

export interface SolverRun {
  id: string;
  label: string;
  timestamp: Date;
  input: SolverInput;
  results: SolverResult[];
}

export type CalendarSource = 'original' | 'solver';
export type CalendarMode = 'pattern' | 'dates';

export interface CalendarEvent {
  id: string;
  title: string;
  start: Date;
  end: Date;
  resource: {
    courseId: string;
    courseName: string;
    classType: string;
    group: number;
    lecturer: string;
    room: string;
    building: string;
    week: 'A' | 'B' | 'AB';
    source: CalendarSource;
    color: string;
  };
}
