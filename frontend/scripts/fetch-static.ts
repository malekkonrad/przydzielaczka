import fs from 'fs/promises';
import path from 'path';
import { parseGroupScheduleHtml, parseClassDetailHtml, rawToClass } from '../src/lib/usos/parser';
import { buildCdydCode, USOS_BASE } from '../src/lib/usos/constants';
import type { CourseGroup, StudyYear } from '../src/types';

interface ProgramEntry {
  program: string;
  name?: string;
  groupCodePrefix: string;
  years: string[];
  sems: number[];
}

const ROOT = process.cwd();
const DATA_DIR = path.join(ROOT, 'public', 'data');
const MAJORS_FILE = path.join(ROOT, 'covered_majors.json');

const FETCH_HEADERS = {
  'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36',
  'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
  'Accept-Language': 'pl-PL,pl;q=0.9',
};

function slugify(year: string) {
  return year.replace('/', '_');
}

function dataFilePath(program: string, year: string, sem: number): string {
  return path.join(DATA_DIR, `${program}-${slugify(year)}-${sem}.json`);
}

async function fileExists(p: string): Promise<boolean> {
  try {
    await fs.access(p);
    return true;
  } catch {
    return false;
  }
}

async function fetchHtml(url: string): Promise<string> {
  const res = await fetch(url, { headers: FETCH_HEADERS });
  if (!res.ok) throw new Error(`HTTP ${res.status} dla ${url}`);
  const text = await res.text();
  if (text.includes('id="loginform"') || text.includes('action="logowanie"')) {
    throw new Error('USOS wymaga zalogowania — ten semestr/kierunek jest niedostępny publicznie.');
  }
  return text;
}

async function fetchSessions(zajCykId: string, grNr: number) {
  const url = `${USOS_BASE}?_action=katalog2/przedmioty/pokazZajecia&zaj_cyk_id=${zajCykId}&gr_nr=${grNr}`;
  try {
    const html = await fetchHtml(url);
    return parseClassDetailHtml(html);
  } catch {
    return [];
  }
}

async function fetchSchedule(
  groupCodePrefix: string,
  year: string,
  sem: number,
): Promise<CourseGroup[]> {
  const groupCode = `${groupCodePrefix}${sem}`;
  const cdydCode = buildCdydCode(year as StudyYear, sem);
  const url = `${USOS_BASE}?_action=katalog2/przedmioty/pokazPlanGrupyPrzedmiotow&grupa_kod=${groupCode}&cdyd_kod=${encodeURIComponent(cdydCode)}`;

  const html = await fetchHtml(url);
  const rawClasses = parseGroupScheduleHtml(html);

  const BATCH = 8;
  const enriched = [];

  for (let i = 0; i < rawClasses.length; i += BATCH) {
    const batch = rawClasses.slice(i, i + BATCH);
    const results = await Promise.all(
      batch.map(async raw => {
        const sessions = raw.zajCykId
          ? await fetchSessions(raw.zajCykId, raw.grNr)
          : [];
        return rawToClass(raw, sessions);
      })
    );
    enriched.push(...results);
    process.stdout.write(`  [${Math.min(i + BATCH, rawClasses.length)}/${rawClasses.length}] zajęcia…\r`);
  }
  if (rawClasses.length > 0) process.stdout.write('\n');

  const byId = new Map<string, CourseGroup>();
  for (const cls of enriched) {
    if (!byId.has(cls.id)) {
      byId.set(cls.id, { id: cls.id, name: cls.name ?? cls.id, classes: [] });
    }
    byId.get(cls.id)!.classes.push(cls);
  }

  return Array.from(byId.values()).sort((a, b) => a.name.localeCompare(b.name, 'pl'));
}

async function main() {
  const raw = await fs.readFile(MAJORS_FILE, 'utf-8');
  const programs: ProgramEntry[] = JSON.parse(raw);

  if (programs.length === 0) {
    console.log('covered_majors.json jest pusty — nic do pobrania.');
    return;
  }

  await fs.mkdir(DATA_DIR, { recursive: true });

  let fetched = 0;
  let skipped = 0;

  for (const prog of programs) {
    for (const year of prog.years) {
      for (const sem of prog.sems) {
        const filePath = dataFilePath(prog.program, year, sem);

        if (await fileExists(filePath)) {
          console.log(`✓ pomiń  ${prog.program} ${year} sem${sem} (plik już istnieje)`);
          skipped++;
          continue;
        }

        console.log(`⬇ pobierz ${prog.program} ${year} sem${sem}  [${prog.groupCodePrefix}${sem}]`);
        try {
          const data = await fetchSchedule(prog.groupCodePrefix, year, sem);
          await fs.writeFile(filePath, JSON.stringify(data));
          console.log(`✓ zapisano ${path.relative(ROOT, filePath)}`);
          fetched++;
        } catch (e) {
          console.error(`✗ błąd   ${prog.program} ${year} sem${sem}: ${e}`);
          process.exit(1);
        }
      }
    }
  }

  console.log(`\nGotowe: ${fetched} pobrano, ${skipped} pominięto.`);
}

main();
