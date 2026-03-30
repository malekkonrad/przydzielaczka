# solver/bindings/wasm/CMakeLists.txt

# To build this folder properly, you should invoke CMake with the Emscripten toolchain:
# emcmake cmake ..

if (EMSCRIPTEN AND BUILD_WASM_BINDINGS)
    # Grab all WebAssembly binding sources
    file(GLOB_RECURSE WASM_BINDING_SOURCES "*.cpp")

    if (WASM_BINDING_SOURCES)
        add_executable(przydzielaczka_wasm ${WASM_BINDING_SOURCES})
        target_link_libraries(przydzielaczka_wasm PRIVATE solver_core)

        # Basic Emscripten flags
        set_target_properties(przydzielaczka_wasm PROPERTIES SUFFIX ".js")
        target_link_options(przydzielaczka_wasm PRIVATE
            "-sNO_EXIT_RUNTIME=1"
            "-sMODULARIZE=1"
            "-sEXPORT_NAME='createPrzydzielaczkaModule'"
            # Add "-lembind" if you're using embind
            "-lembind"
        )
    endif()
else()
    if(BUILD_WASM_BINDINGS)
        message(WARNING "Emscripten not found but BUILD_WASM_BINDINGS is ON. Make sure you run 'emcmake cmake ..'")
    endif()
endif()
