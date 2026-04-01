//
// Created by mateu on 01.04.2026.
//

#include "simple_solver.h"
#include "constraint_evaluator.h"
#include "policies/int_time_policy.h"
#include "policies/bitset_time_policy.h"

// Explicit instantiations so the linker finds both variants.
template class SimpleSolver<ConstraintEvaluator<IntTimePolicy>>;
template class SimpleSolver<ConstraintEvaluator<BitsetTimePolicy>>;
