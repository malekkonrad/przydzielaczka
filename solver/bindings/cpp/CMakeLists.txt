# solver/bindings/cpp/CMakeLists.txt

# This folder is meant for a raw C++ interface, e.g. a command line wrapper or standalone library.
# Here we define an executable 'solver_cli' just as an example.
# You can change it to a library if you want to export C++ bindings instead.

# Create a small static library or executable
add_executable(solver_cli main.cpp
        main.cpp)
# target_link_libraries(solver_cli PRIVATE solver_core)

# If this folder is meant to export a C++ interface library:
# add_library(solver_cpp_bindings STATIC bindings.cpp)
# target_link_libraries(solver_cpp_bindings PUBLIC solver_core)
