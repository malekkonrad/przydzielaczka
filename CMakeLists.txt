cmake_minimum_required(VERSION 3.28.3)

project(przydzielaczka
        VERSION 0.0.1
        DESCRIPTION "Przydzielaczka constraints solver"
        LANGUAGES CXX
)

# --- General Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Useful for bindings

# --- Compiler cache (ccache / sccache) ---
# ccache works well with GCC and Clang (including on Windows/MinGW).
# sccache is the better choice on Windows with MSVC.
# Install via: winget install ccache   OR   winget install Mozilla.sccache
find_program(CCACHE_PROGRAM ccache)
find_program(SCCACHE_PROGRAM sccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Compiler cache: ccache (${CCACHE_PROGRAM})")
elseif(SCCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    message(STATUS "Compiler cache: sccache (${SCCACHE_PROGRAM})")
else()
    message(STATUS "Compiler cache: none found (install ccache or sccache for faster rebuilds)")
endif()

# --- Options ---
option(BUILD_TESTING "Build the testing tree" OFF)
option(BUILD_PYTHON_BINDINGS "Build Python bindings via pybind11" OFF)
# Emscripten / WebAssembly builds are typically controlled by CMake toolchains,
# but we can provide an option to specifically toggle the WASM targets.
option(BUILD_WASM_BINDINGS "Build WebAssembly bindings" OFF)

# --- Dependencies ---
find_package(nlohmann_json CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(
    json_schema_validator
    GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
    GIT_TAG        2.3.0
)
FetchContent_MakeAvailable(json_schema_validator)

# --- Subdirectories ---
add_subdirectory(solver)

# Only add tests subfolder if we want testing enabled
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()