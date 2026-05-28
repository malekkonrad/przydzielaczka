import { NextRequest, NextResponse } from 'next/server';
import { parseClassDetailHtml } from '@/lib/usos/parser';
import { USOS_BASE } from '@/lib/usos/constants';

export async function GET(req: NextRequest) {
  const { searchParams } = req.nextUrl;
  const zajCykId = searchParams.get('zaj_cyk_id');
  const grNr     = searchParams.get('gr_nr');
  if (!zajCykId || !grNr) {
    return NextResponse.json({ error: 'Missing params' }, { status: 400 });
  }

  const url = `${USOS_BASE}?_action=katalog2/przedmioty/pokazZajecia&zaj_cyk_id=${zajCykId}&gr_nr=${grNr}`;
  const cookie = req.headers.get('x-usos-cookie') ?? undefined;
  const headers: Record<string, string> = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36',
    'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
    'Accept-Language': 'pl-PL,pl;q=0.9,en;q=0.8',
    'Accept-Encoding': 'gzip, deflate, br',
    'Connection': 'keep-alive',
  };
  if (cookie) headers['Cookie'] = cookie;

  try {
    const res = await fetch(url, { headers, redirect: 'follow' });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const html = await res.text();
    const sessions = parseClassDetailHtml(html);
    return NextResponse.json(sessions);
  } catch (err) {
    return NextResponse.json({ error: String(err) }, { status: 502 });
  }
}
