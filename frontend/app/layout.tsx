import type { Metadata } from 'next';
import Providers from './providers';
import './globals.css';
import 'react-big-calendar/lib/css/react-big-calendar.css';

export const metadata: Metadata = {
  title: 'Przydzielaczka',
  description: 'Plan zajęć AGH z optymalizatorem',
};

const isStatic = process.env.NEXT_PUBLIC_STATIC === 'true';
const basePath = process.env.NEXT_PUBLIC_BASE_PATH ?? '';

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="pl">
      <head>
        {/* Needed on GitHub Pages to enable SharedArrayBuffer for WASM solver */}
        {isStatic && <script src={`${basePath}/coi-serviceworker.js`} />}
      </head>
      <body>
        <Providers>{children}</Providers>
      </body>
    </html>
  );
}
