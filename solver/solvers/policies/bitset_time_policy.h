//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <bitset>

// Time represented as a bitset of 15-minute slots covering 7:00–21:30.
// 14h 30m / 15 min = 58 slots.
// Overlap is a bitwise AND — no branching, cache-friendly.
struct BitsetTimePolicy
{
    static constexpr int SLOT_MIN  = 15;
    static constexpr int DAY_START = 7 * 60;          // 420 min
    static constexpr int DAY_END   = 21 * 60 + 30;    // 1290 min
    static constexpr int NUM_SLOTS = (DAY_END - DAY_START) / SLOT_MIN;  // 58

    using TimeType = std::bitset<NUM_SLOTS>;

    // Sets bits for every slot in [start_min, end_min).
    static TimeType encode(int start_min, int end_min)
    {
        TimeType bits;
        int first = (start_min - DAY_START) / SLOT_MIN;
        int last  = (end_min   - DAY_START) / SLOT_MIN;
        for (int i = first; i < last; ++i)
            if (i >= 0 && i < NUM_SLOTS)
                bits.set(i);
        return bits;
    }

    static bool overlaps_time(TimeType a, TimeType b)
    {
        return (a & b).any();
    }

    // True if any occupied slot falls within [block_start, block_end).
    static bool time_in_block(TimeType t, int block_start, int block_end)
    {
        return (t & encode(block_start, block_end)).any();
    }
};
