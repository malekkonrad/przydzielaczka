import type { StudyYear, SemesterType } from '@/types';

export interface StudyProgram {
  code: string;
  name: string;
  groupCodePrefix: string;
  years: number;
}

export const STUDY_PROGRAMS: StudyProgram[] = [
  { code: 'ISI', name: 'Informatyka Stosowana i Inżynieria (ISI)', groupCodePrefix: 'ISI_1S_sem_',     years: 7 },
  { code: 'AiR', name: 'Automatyka i Robotyka (AiR)',               groupCodePrefix: 'AiR_1S_sem_',     years: 7 },
  { code: 'EiT', name: 'Elektronika i Telekomunikacja (EiT)',        groupCodePrefix: '230-EiT_1S_sem',  years: 7 },
  { code: 'EIT', name: 'EIT',                                        groupCodePrefix: 'EIT_1S_sem_',     years: 7 },
];

export const STUDY_YEARS: StudyYear[] = ['23/24', '24/25', '25/26', '26/27'];

export const SEMESTER_TYPE: Record<number, SemesterType> = {
  1: 'Z', 2: 'L', 3: 'Z', 4: 'L', 5: 'Z', 6: 'L', 7: 'Z',
};

/** Constructs the USOS group code using the program's prefix, e.g. ISI_1S_sem_6 or 230-EiT_1S_sem7 */
export function buildGroupCode(program: string, semesterNumber: number): string {
  const prefix = STUDY_PROGRAMS.find(p => p.code === program)?.groupCodePrefix
    ?? `${program}_1S_sem_`;
  return `${prefix}${semesterNumber}`;
}

/** Constructs the USOS cdyd_kod, e.g. "25/26-L" */
export function buildCdydCode(year: StudyYear, semesterNumber: number): string {
  const type = SEMESTER_TYPE[semesterNumber] ?? 'Z';
  return `${year}-${type}`;
}

export const USOS_BASE = 'https://web.usos.agh.edu.pl/kontroler.php';

export const DAY_NAMES: Record<string, number> = {
  'Poniedziałek': 1,
  'Wtorek': 2,
  'Środa': 3,
  'Czwartek': 4,
  'Piątek': 5,
  'Sobota': 6,
  'Niedziela': 0,
};

export const DAY_LABELS: Record<number, string> = {
  1: 'Poniedziałek',
  2: 'Wtorek',
  3: 'Środa',
  4: 'Czwartek',
  5: 'Piątek',
  6: 'Sobota',
  0: 'Niedziela',
};

export const CLASS_TYPE_LABELS: Record<string, string> = {
  W:   'Wykład',
  CWL: 'Ćwiczenia lab.',
  CWP: 'Ćwiczenia proj.',
  CW:  'Ćwiczenia',
  SEM: 'Seminarium',
  LAB: 'Laboratorium',
};

// Colour palette for courses (MUI-ish)
export const COURSE_COLORS = [
  '#1976d2', '#388e3c', '#d32f2f', '#7b1fa2', '#f57c00',
  '#0288d1', '#558b2f', '#c62828', '#6a1b9a', '#ef6c00',
  '#00838f', '#2e7d32', '#b71c1c', '#4a148c', '#e65100',
  '#00695c', '#283593', '#ad1457', '#37474f', '#827717',
];
