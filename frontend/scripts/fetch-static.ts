import fs from 'fs/promises';
import path from 'path';
import { Agent, setGlobalDispatcher } from 'undici';
import { parseGroupScheduleHtml, parseClassDetailHtml, rawToClass } from '../src/lib/usos/parser';
import { buildCdydCode, USOS_BASE } from '../src/lib/usos/constants';
import type { CourseGroup, StudyYear } from '../src/types';

// AGH USOS may use older SSL configuration that fails strict cert verification in CI
setGlobalDispatcher(new Agent({
  connect: { rejectUnauthorized: false },
  connectTimeout: 30_000,
  keepAliveTimeout: 30_000,
}));

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

const RETRY_COUNT = 3;
const RETRY_DELAY_MS = 3_000;

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

async function fetchWithRetry(url: string): Promise<string> {
  let lastErr: unknown;
  for (let attempt = 1; attempt <= RETRY_COUNT; attempt++) {
    try {
      const res = await fetch(url, {
        signal: AbortSignal.timeout(30_000),
      });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const text = await res.text();
      if (text.includes('id="loginform"') || text.includes('action="logowanie"')) {
        throw new Error('USOS wymaga zalogowania — ten semestr/kierunek jest niedostępny publicznie.');
      }
      return text;
    } catch (err) {
      lastErr = err;
      const cause = (err as any)?.cause;
      const detail = cause ? ` (${cause})` : '';
      if (attempt < RETRY_COUNT) {
        console.warn(`  próba ${attempt}/${RETRY_COUNT} nieudana${detail}, ponawiam za ${RETRY_DELAY_MS / 1000}s…`);
        await new Promise(r => setTimeout(r, RETRY_DELAY_MS));
      }
    }
  }
  const cause = (lastErr as any)?.cause;
  throw new Error(`Fetch nieudany po ${RETRY_COUNT} próbach${cause ? `: ${cause}` : `: ${lastErr}`}`);
}

async function fetchSessions(zajCykId: string, grNr: number) {
  const url = `${USOS_BASE}?_action=katalog2/przedmioty/pokazZajecia&zaj_cyk_id=${zajCykId}&gr_nr=${grNr}`;
  try {
    const html = await fetchWithRetry(url);
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

  const html = await fetchWithRetry(url);
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
  const failed: string[] = [];

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
          failed.push(`${prog.program} ${year} sem${sem}`);
        }
      }
    }
  }

  console.log(`\nGotowe: ${fetched} pobrano, ${skipped} pominięto, ${failed.length} błędów.`);
  if (failed.length > 0) {
    console.error(`Nieudane wpisy:\n${failed.map(f => `  - ${f}`).join('\n')}`);
    process.exit(1);
  }
}

main();
