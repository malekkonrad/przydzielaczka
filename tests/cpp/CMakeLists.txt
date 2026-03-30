# tests/cpp/CMakeLists.txt

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Gather testing sources
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# If there are no cpp files, don't generate the test target
if (TEST_SOURCES)
    add_executable(przydzielaczka_cpp_tests ${TEST_SOURCES})

    # Link against googletest, nlohmann_json, and our own solver component
    target_link_libraries(przydzielaczka_cpp_tests PRIVATE
        GTest::gtest_main
        solver_core
        nlohmann_json::nlohmann_json
    )

    include(GoogleTest)
    gtest_discover_tests(przydzielaczka_cpp_tests)
else()
    message(STATUS "No testing sources found in tests/cpp. Skipping C++ test target generation.")
endif()
