'use client';

import { Box, Divider } from '@mui/material';
import ScheduleSelector from './ScheduleSelector';
import CourseList from './CourseList';

export default function Sidebar() {
  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%' }}>
      <ScheduleSelector />
      <Divider />
      <Box sx={{ flex: 1, overflow: 'auto' }}>
        <CourseList />
      </Box>
    </Box>
  );
}
