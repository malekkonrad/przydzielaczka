# solver/bindings/CMakeLists.txt

add_subdirectory(cpp)

if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

if(BUILD_WASM_BINDINGS)
    add_subdirectory(wasm)
endif()
