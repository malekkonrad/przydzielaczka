# solver/core/CMakeLists.txt

# Find all source files for the core library
file(GLOB_RECURSE CORE_SOURCES "src/*.cpp")

# Define our main core library
# Using STATIC or SHARED based on BUILD_SHARED_LIBS, or force STATIC
add_library(solver_core STATIC ${CORE_SOURCES}
        src/time_table_state.cpp
        include/time_table_state.h
        include/input_data_models.h
        include/solver_data_models.h
        ../solvers/simple_solver.cpp
        ../solvers/simple_solver.h)

# Specify include directories correctly
# Public includes are available to anyone linking to solver_core
target_include_directories(solver_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Private includes are only for compiling solver_core itself
target_include_directories(solver_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
)

# Provide the schema path as a compile-time constant so the validator can load it
target_compile_definitions(solver_core PRIVATE
    INPUT_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/shared/schemas/input.schema.json"
)
