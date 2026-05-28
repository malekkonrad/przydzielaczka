# solver/bindings/python/CMakeLists.txt
#
# Enabled via top-level option:
#   cmake .. -DBUILD_PYTHON_BINDINGS=ON
#
# Output: przydzielaczka_py.<ext>  (e.g. przydzielaczka_py.cpython-311-x86_64-linux-gnu.so)

find_package(pybind11 CONFIG REQUIRED)

file(GLOB_RECURSE PYTHON_BINDING_SOURCES "*.cpp")

if (PYTHON_BINDING_SOURCES)
    pybind11_add_module(przydzielaczka_py ${PYTHON_BINDING_SOURCES})
    target_link_libraries(przydzielaczka_py PRIVATE solver_core)
    # Output .pyd/.so directly into the build root for easy PYTHONPATH setup.
    set_target_properties(przydzielaczka_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
else()
    message(STATUS "No .cpp files found for Python bindings — skipping pybind11 module.")
endif()