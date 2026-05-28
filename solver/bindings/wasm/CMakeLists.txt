# solver/bindings/wasm/CMakeLists.txt
#
# Build with the Emscripten toolchain:
#   emcmake cmake .. -DBUILD_WASM_BINDINGS=ON
#   emmake make
#
# Output: przydzielaczka_wasm.js + przydzielaczka_wasm.wasm

if (EMSCRIPTEN AND BUILD_WASM_BINDINGS)
    file(GLOB_RECURSE WASM_BINDING_SOURCES "*.cpp")

    if (WASM_BINDING_SOURCES)
        add_executable(przydzielaczka_wasm ${WASM_BINDING_SOURCES})
        target_link_libraries(przydzielaczka_wasm PRIVATE solver_core)

        set_target_properties(przydzielaczka_wasm PROPERTIES SUFFIX ".js")

        target_link_options(przydzielaczka_wasm PRIVATE
            "-sNO_EXIT_RUNTIME=1"
            "-sMODULARIZE=1"
            "-sEXPORT_NAME='createPrzydzielaczkaModule'"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sENVIRONMENT=web,worker,node"
            "-lembind"
            "--embed-file=${CMAKE_SOURCE_DIR}/shared/schemas/input.schema.json@/input.schema.json"
            "-sASSERTIONS=1"
            "-sNO_DISABLE_EXCEPTION_CATCHING"
        )
    endif()
else()
    if (BUILD_WASM_BINDINGS)
        message(WARNING "BUILD_WASM_BINDINGS is ON but Emscripten was not detected. "
                        "Run CMake via 'emcmake cmake ..'")
    endif()
endif()