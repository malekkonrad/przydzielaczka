import { load } from 'cheerio';
import type { CourseClass, Session } from '@/types';
import { DAY_NAMES } from './constants';

// ─── helpers ──────────────────────────────────────────────────────────────────

function parseGridTime(gridValue: string): number {
  const m = gridValue.match(/g(\d{2})(\d{2})/);
  if (!m) return 0;
  return parseInt(m[1]) * 60 + parseInt(m[2]);
}

function parseTimeStr(t: string): number {
  // "09:45" → 585
  const [h, min] = t.split(':').map(Number);
  return h * 60 + min;
}

// ─── Group-schedule page ──────────────────────────────────────────────────────

export interface ParsedClassRaw {
  name: string;
  courseId: string;
  startTime: number;
  endTime: number;
  day: number;
  week: 'A' | 'B' | 'AB';
  classType: string;
  group: number;
  room: string;
  building: string;
  lecturer: string;
  zajCykId: string;
  grNr: number;
}

export function parseGroupScheduleHtml(html: string): ParsedClassRaw[] {
  const $ = load(html);
  const classes: ParsedClassRaw[] = [];

  $('timetable-day').each((_i, dayEl) => {
    const prevDiv = $(dayEl).prev('div');
    const dayName = prevDiv.find('h4').text().trim();
    const day = DAY_NAMES[dayName] ?? 1;

    $(dayEl).find('timetable-entry').each((_j, entryEl) => {
      const entry = $(entryEl);

      const name = entry.attr('name') ?? '';
      const courseId = entry.attr('name-id') ?? '';
      const style = entry.attr('style') ?? '';

      const startMatch = style.match(/grid-row-start:\s*(g\d+)/);
      const endMatch   = style.match(/grid-row-end:\s*(g\d+)/);
      const startTime  = startMatch ? parseGridTime(startMatch[1]) : 0;
      const endTime    = endMatch   ? parseGridTime(endMatch[1])   : 0;

      // "CWL, gr. 2 (208, bud. C2)"
      const infoText = entry.find('[slot="info"]').text().replace(/ /g, ' ').trim();
      const infoMatch = infoText.match(/^(\S+),\s*gr\.?\s*(\d+)\s*\(([^,]+),\s*bud\.\s*([^)]+)\)/);
      const classType = infoMatch?.[1] ?? '';
      const group     = infoMatch ? parseInt(infoMatch[2]) : 1;
      const room      = infoMatch?.[3]?.trim() ?? '';
      const building  = infoMatch?.[4]?.trim() ?? '';

      // Week type from one.svg / two.svg
      const timeHtml = entry.find('[slot="time"]').html() ?? '';
      const hasOne = timeHtml.includes('one.svg');
      const hasTwo = timeHtml.includes('two.svg');
      const week: 'A' | 'B' | 'AB' = hasOne && !hasTwo ? 'A' : hasTwo && !hasOne ? 'B' : 'AB';

      // Links for detail page
      const dialogHref = entry.find('[slot="dialog-info"] a').attr('href') ?? '';
      const zajMatch = dialogHref.match(/zaj_cyk_id=(\d+)/);
      const grMatch  = dialogHref.match(/gr_nr=(\d+)/);
      const zajCykId = zajMatch?.[1] ?? '';
      const grNr     = grMatch ? parseInt(grMatch[1]) : group;

      const lecturer = entry.find('[slot="dialog-person"] a').first().text().trim();

      if (!courseId) return;

      classes.push({
        name,
        courseId,
        startTime,
        endTime,
        day,
        week,
        classType,
        group,
        room,
        building,
        lecturer,
        zajCykId,
        grNr,
      });
    });
  });

  return classes;
}

// ─── Class-detail page (session dates) ───────────────────────────────────────

export function parseClassDetailHtml(html: string): Session[] {
  const $ = load(html);
  const sessions: Session[] = [];

  $('#lista_dat_spotkan tbody tr').each((_i, row) => {
    const td = $(row).find('td').first();

    // Date from first anchor text
    const date = td.find('a').first().text().trim();
    if (!date.match(/^\d{4}-\d{2}-\d{2}$/)) return;

    // Time: "09:45 : 11:15" buried in the text node
    const fullText = td.text().replace(/\s+/g, ' ').trim();
    const timeMatch = fullText.match(/(\d{2}:\d{2})\s*:\s*(\d{2}:\d{2})/);
    const start_time = timeMatch ? parseTimeStr(timeMatch[1]) : 0;
    const end_time   = timeMatch ? parseTimeStr(timeMatch[2]) : 0;

    // Room from second anchor ("sala 403" → "403")
    const salaText = td.find('a').eq(1).text().trim().replace(/^sala\s*/i, '');
    const room = salaText || '';

    // Building from .note span
    const building = td.find('.note').text().trim();

    sessions.push({
      date,
      location: { room, building },
      start_time,
      end_time,
    });
  });

  return sessions;
}

// ─── Convert raw → CourseClass ────────────────────────────────────────────────

export function rawToClass(raw: ParsedClassRaw, sessions: Session[]): CourseClass {
  return {
    id: raw.courseId,
    name: raw.name,
    lecturer: raw.lecturer,
    day: raw.day,
    week: raw.week,
    location: { room: raw.room, building: raw.building },
    group: raw.group,
    class_type: raw.classType,
    start_time: raw.startTime,
    end_time: raw.endTime,
    sessions,
    zajCykId: raw.zajCykId,
  };
}
