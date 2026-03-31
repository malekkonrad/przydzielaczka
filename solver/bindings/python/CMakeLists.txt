# solver/bindings/python/CMakeLists.txt

find_package(pybind11 CONFIG REQUIRED)

# Grab all the python cpp sources (e.g. module definition files)
file(GLOB_RECURSE PYTHON_BINDING_SOURCES "*.cpp")

# Depending on if there are sources, we create the module. 
# Usually a python binding requires at least one source file.
if (PYTHON_BINDING_SOURCES)
    pybind11_add_module(przydzielaczka_py ${PYTHON_BINDING_SOURCES})
    target_link_libraries(przydzielaczka_py PRIVATE solver_core)
else()
    message(STATUS "No cpp files found for python bindings. Skipping pybind11 module generation.")
endif()
