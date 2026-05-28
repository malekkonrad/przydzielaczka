'use client';

import { Box, useTheme } from '@mui/material';
import Sidebar from '@/components/sidebar/Sidebar';
import TimetableCalendar from '@/components/calendar/TimetableCalendar';
import SolverPanel from '@/components/solver/SolverPanel';
import AppBar from './AppBar';

const SIDEBAR_W  = 280;
const SOLVER_W   = 320;

export default function AppLayout() {
  const theme = useTheme();

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100vh', overflow: 'hidden' }}>
      <AppBar />
      <Box sx={{ display: 'flex', flex: 1, overflow: 'hidden' }}>
        {/* Left sidebar */}
        <Box
          component="aside"
          sx={{
            width: SIDEBAR_W,
            flexShrink: 0,
            borderRight: `1px solid ${theme.palette.divider}`,
            overflowY: 'auto',
            overflowX: 'hidden',
            bgcolor: 'background.paper',
          }}
        >
          <Sidebar />
        </Box>

        {/* Calendar (grows) */}
        <Box sx={{ flex: 1, overflow: 'hidden', display: 'flex', flexDirection: 'column' }}>
          <TimetableCalendar />
        </Box>

        {/* Right solver panel */}
        <Box
          component="aside"
          sx={{
            width: SOLVER_W,
            flexShrink: 0,
            borderLeft: `1px solid ${theme.palette.divider}`,
            overflowY: 'auto',
            bgcolor: 'background.paper',
          }}
        >
          <SolverPanel />
        </Box>
      </Box>
    </Box>
  );
}
