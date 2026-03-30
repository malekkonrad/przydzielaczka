# solver/core/CMakeLists.txt

# Find all source files for the core library
file(GLOB_RECURSE CORE_SOURCES "src/*.cpp")

# Define our main core library
# Using STATIC or SHARED based on BUILD_SHARED_LIBS, or force STATIC
add_library(solver_core STATIC ${CORE_SOURCES})

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

# Link dependencies (nlohmann_json)
# If it's used in headers of solver_core, use PUBLIC, else PRIVATE. Let's assume PUBLIC.
target_link_libraries(solver_core PUBLIC nlohmann_json::nlohmann_json)
