import { createTheme, type PaletteMode } from '@mui/material';

export function buildTheme(mode: PaletteMode) {
  return createTheme({
    palette: {
      mode,
      primary:   { main: '#1976d2' },
      secondary: { main: '#9c27b0' },
      background: {
        default: mode === 'dark' ? '#0f1117' : '#f5f5f5',
        paper:   mode === 'dark' ? '#1a1d27' : '#ffffff',
      },
    },
    typography: {
      fontFamily: '"Inter", "Roboto", "Helvetica", "Arial", sans-serif',
      fontSize: 13,
    },
    shape: { borderRadius: 8 },
    components: {
      MuiPaper: {
        styleOverrides: {
          root: { backgroundImage: 'none' },
        },
      },
      MuiButton: {
        defaultProps: { disableElevation: true },
      },
    },
  });
}
