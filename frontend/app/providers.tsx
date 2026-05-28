'use client';

import { useMemo } from 'react';
import { ThemeProvider, CssBaseline } from '@mui/material';
import { buildTheme } from '@/theme/theme';
import { useAppStore } from '@/store/appStore';

export default function Providers({ children }: { children: React.ReactNode }) {
  const darkMode = useAppStore(s => s.darkMode);
  const theme = useMemo(() => buildTheme(darkMode ? 'dark' : 'light'), [darkMode]);

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      {children}
    </ThemeProvider>
  );
}
