'use client';

import {
  AppBar as MuiAppBar,
  Toolbar,
  Typography,
  IconButton,
  Tooltip,
} from '@mui/material';
import { DarkMode, LightMode, CalendarMonth } from '@mui/icons-material';
import { useAppStore } from '@/store/appStore';

export default function AppBar() {
  const darkMode = useAppStore(s => s.darkMode);
  const toggle   = useAppStore(s => s.toggleDarkMode);

  return (
    <MuiAppBar position="static" elevation={0} color="default"
      sx={{ borderBottom: '1px solid', borderColor: 'divider', zIndex: 10 }}>
      <Toolbar variant="dense" sx={{ gap: 1 }}>
        <CalendarMonth fontSize="small" color="primary" />
        <Typography variant="subtitle1" fontWeight={600} sx={{ flex: 1 }}>
          Przydzielaczka
        </Typography>
        <Tooltip title={darkMode ? 'Tryb jasny' : 'Tryb ciemny'}>
          <IconButton size="small" onClick={toggle}>
            {darkMode ? <LightMode fontSize="small" /> : <DarkMode fontSize="small" />}
          </IconButton>
        </Tooltip>
      </Toolbar>
    </MuiAppBar>
  );
}
