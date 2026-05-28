import type { Metadata } from 'next';
import Providers from './providers';
import './globals.css';
import 'react-big-calendar/lib/css/react-big-calendar.css';

export const metadata: Metadata = {
  title: 'Przydzielaczka',
  description: 'Plan zajęć AGH z optymalizatorem',
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="pl">
      <body>
        <Providers>{children}</Providers>
      </body>
    </html>
  );
}
