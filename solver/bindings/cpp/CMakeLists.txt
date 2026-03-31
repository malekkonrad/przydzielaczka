# solver/bindings/cpp/CMakeLists.txt

add_executable(solver_cli main.cpp)

target_link_libraries(solver_cli PRIVATE solver_core)

# Expose the sample input path as a compile-time constant (same pattern as INPUT_SCHEMA_PATH).
target_compile_definitions(solver_cli PRIVATE
    SAMPLE_INPUT_PATH="${CMAKE_SOURCE_DIR}/tests/data/sample_input.json"
)
