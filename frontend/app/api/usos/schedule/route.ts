import { NextRequest, NextResponse } from 'next/server';
import { parseGroupScheduleHtml, parseClassDetailHtml, rawToClass } from '@/lib/usos/parser';
import { buildGroupCode, buildCdydCode, USOS_BASE } from '@/lib/usos/constants';
import type { CourseClass, CourseGroup, StudyYear } from '@/types';

export const dynamic = 'force-static';

const FETCH_HEADERS = {
  'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36',
  'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8',
  'Accept-Language': 'pl-PL,pl;q=0.9,en-US;q=0.8,en;q=0.7',
  'Accept-Encoding': 'gzip, deflate, br',
  'Connection': 'keep-alive',
  'Upgrade-Insecure-Requests': '1',
  'Sec-Fetch-Dest': 'document',
  'Sec-Fetch-Mode': 'navigate',
  'Sec-Fetch-Site': 'none',
  'Cache-Control': 'max-age=0',
};

async function fetchHtml(url: string, cookie?: string): Promise<string> {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`USOS zwrócił HTTP ${res.status}. Spróbuj podać cookie sesji.`);
  const text = await res.text();
  // Detect login redirect (USOS sends 200 with login form when session expired)
  if (text.includes('id="loginform"') || text.includes('action="logowanie"')) {
    throw new Error('USOS wymaga zalogowania. Podaj cookie sesji (np. PHPSESSID=...) w polu "Cookie USOS".');
  }
  return text;
}

async function fetchSessions(
  zajCykId: string,
  grNr: number,
  cookie?: string
) {
  const url = `${USOS_BASE}?_action=katalog2/przedmioty/pokazZajecia&zaj_cyk_id=${zajCykId}&gr_nr=${grNr}`;
  try {
    const html = await fetchHtml(url, cookie);
    return parseClassDetailHtml(html);
  } catch {
    return [];
  }
}

export async function GET(req: NextRequest) {
  if (process.env.STATIC_BUILD === 'true') return NextResponse.json([]);

  const { searchParams } = req.nextUrl;
  const program = searchParams.get('program') ?? 'ISI';
  const year    = (searchParams.get('year') ?? '25/26') as StudyYear;
  const semNum  = parseInt(searchParams.get('sem') ?? '6');
  const cookie  = req.headers.get('x-usos-cookie') ?? undefined;

  const groupCode = buildGroupCode(program, semNum);
  const cdydCode  = buildCdydCode(year, semNum);
  const url = `${USOS_BASE}?_action=katalog2/przedmioty/pokazPlanGrupyPrzedmiotow&grupa_kod=${groupCode}&cdyd_kod=${encodeURIComponent(cdydCode)}`;

  let html: string;
  try {
    html = await fetchHtml(url, cookie);
  } catch (err) {
    return NextResponse.json({ error: String(err) }, { status: 502 });
  }

  const rawClasses = parseGroupScheduleHtml(html);

  // Fetch session dates in parallel (batches of 8)
  const BATCH = 8;
  const enriched: CourseClass[] = [];

  for (let i = 0; i < rawClasses.length; i += BATCH) {
    const batch = rawClasses.slice(i, i + BATCH);
    const results = await Promise.all(
      batch.map(async raw => {
        const sessions = raw.zajCykId
          ? await fetchSessions(raw.zajCykId, raw.grNr, cookie)
          : [];
        return rawToClass(raw, sessions);
      })
    );
    enriched.push(...results);
  }

  // Group by courseId + name
  const byId = new Map<string, CourseGroup>();
  for (const cls of enriched) {
    if (!byId.has(cls.id)) {
      byId.set(cls.id, { id: cls.id, name: cls.name ?? cls.id, classes: [] });
    }
    byId.get(cls.id)!.classes.push(cls);
  }

  const groups: CourseGroup[] = Array.from(byId.values()).sort((a, b) =>
    a.name.localeCompare(b.name, 'pl')
  );

  return NextResponse.json(groups);
}
