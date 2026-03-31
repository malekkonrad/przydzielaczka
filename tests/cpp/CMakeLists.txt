# tests/cpp/CMakeLists.txt

find_package(GTest CONFIG REQUIRED)

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
