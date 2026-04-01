//
// Created by mateu on 01.04.2026.
//

#pragma once

// Time represented as two plain ints (minutes from midnight).
// Overlap is a standard interval intersection check.
struct IntTimePolicy
{
    struct TimeType
    {
        int start;
        int end;
    };

    static TimeType encode(int start_min, int end_min)
    {
        return { start_min, end_min };
    }

    static bool overlaps_time(TimeType a, TimeType b)
    {
        return a.start < b.end && b.start < a.end;
    }

    // True if the class interval overlaps the block [block_start, block_end).
    static bool time_in_block(TimeType t, int block_start, int block_end)
    {
        return t.start < block_end && block_start < t.end;
    }
};
