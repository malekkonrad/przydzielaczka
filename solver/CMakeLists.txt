# solver/CMakeLists.txt

add_subdirectory(bindings)

# Find all source files for the core library
file(GLOB_RECURSE CORE_SOURCES "core/src/*.cpp")

# Define our main core library
add_library(solver_core STATIC ${CORE_SOURCES}
        core/src/time_table_state.cpp
        core/include/time_table_state.h
        core/include/data_models.h
        utils/tuple_hash.h
        core/include/config.h
        solvers/simple_full_solver.h
        core/include/constraint_variant_fwd.h
        solvers/optimized_full_solver.h
        core/src/sequence_context.cpp
        core/include/sequence_context.h
        solvers/policies/policies.h
        solvers/policies/int_absence_policy.h
        solvers/policies/null_policy.h
        solvers/traits.h
        solvers/stats.h
        solvers/stats.cpp
        solvers/class_group_range.h
)

# Public includes are available to anyone linking to solver_core
target_include_directories(solver_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<INSTALL_INTERFACE:include>
)

# Private includes are only for compiling solver_core itself
target_include_directories(solver_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core/src
    ${CMAKE_CURRENT_SOURCE_DIR}/solvers
)

# Link dependencies
target_link_libraries(solver_core PUBLIC
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator
)

# Precompiled headers — compiled once, reused by every .cpp in solver_core.
# nlohmann/json.hpp is the heaviest include; PCH gives the biggest speedup here.
target_precompile_headers(solver_core PRIVATE
    <nlohmann/json.hpp>
    <nlohmann/json-schema.hpp>
    <algorithm>
    <map>
    <optional>
    <string>
    <unordered_set>
    <variant>
    <vector>
    <ranges>
)

# Provide the schema path as a compile-time constant so the validator can load it
target_compile_definitions(solver_core PRIVATE
    INPUT_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/shared/schemas/input.schema.json"
)
