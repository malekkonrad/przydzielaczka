import type { CourseGroup, StudyYear } from '@/types';

export const IS_STATIC = process.env.NEXT_PUBLIC_STATIC === 'true';

function slugify(year: string) {
  return year.replace('/', '_');
}

export async function fetchScheduleData(
  program: string,
  year: StudyYear,
  sem: number,
  cookie?: string,
): Promise<CourseGroup[]> {
  if (IS_STATIC) {
    const base = process.env.NEXT_PUBLIC_BASE_PATH ?? '';
    const res = await fetch(`${base}/data/${program}-${slugify(year)}-${sem}.json`);
    if (!res.ok) {
      throw new Error(`Brak danych statycznych dla ${program} ${year} sem${sem}. Uruchom skrypt fetch-static lub dodaj kierunek do covered_majors.json.`);
    }
    return res.json();
  }

  const params = new URLSearchParams({ program, year, sem: String(sem) });
  const headers: Record<string, string> = {};
  if (cookie) headers['x-usos-cookie'] = cookie;

  const res = await fetch(`/api/usos/schedule?${params}`, { headers });
  if (!res.ok) {
    const body = await res.json().catch(() => ({}));
    throw new Error(body.error ?? `HTTP ${res.status}`);
  }
  return res.json();
}
