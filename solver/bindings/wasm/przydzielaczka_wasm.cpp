#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <config.h>
#include <solver_runner.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

using namespace emscripten;

// JavaScript-friendly config wrapper.
// solution_callback is a JS function that receives a parsed JS object per solution found.
struct WasmConfig {
    size_t max_keep_solutions    = 10000;
    size_t max_solutions         = 10;
    size_t max_runtime           = 10000;
    bool   verbose               = false;
    bool   early_stopping        = false;
    bool   simplified_evaluation = false;
    val    solution_callback     = val::undefined();
};

// Wraps SolverRunner: run() accepts/returns JSON strings for easy JS interop.
class WasmSolverRunner {
public:
    explicit WasmSolverRunner(const WasmConfig& wasm_cfg) {
        input_models::config cfg;
        cfg.max_keep_solutions    = wasm_cfg.max_keep_solutions;
        cfg.max_solutions         = wasm_cfg.max_solutions;
        cfg.max_runtime           = wasm_cfg.max_runtime;
        cfg.verbose               = wasm_cfg.verbose;
        cfg.early_stopping        = wasm_cfg.early_stopping;
        cfg.simplified_evaluation = wasm_cfg.simplified_evaluation;

        if (!wasm_cfg.solution_callback.isNull() &&
            !wasm_cfg.solution_callback.isUndefined()) {
            val cb = wasm_cfg.solution_callback;
            cfg.solution_callback = [cb](const std::string& json_str) {
                // Parse JSON string on the JS side and pass the resulting JS object.
                val parsed = val::global("JSON").call<val>("parse", json_str);
                cb(parsed);
            };
        }

        runner_ = std::make_unique<SolverRunner>(cfg);
    }

    // Accepts a JSON string, returns a JSON string.
    std::string run(const std::string& input_json) {
        return runner_->run(nlohmann::json::parse(input_json)).dump();
    }

    // Loads input from a file path accessible inside the WASM virtual filesystem.
    std::string run_file(const std::string& path) {
        return runner_->run(path).dump();
    }

private:
    std::unique_ptr<SolverRunner> runner_;
};

EMSCRIPTEN_BINDINGS(przydzielaczka) {
    class_<WasmConfig>("Config")
        .constructor<>()
        .property("max_keep_solutions",    &WasmConfig::max_keep_solutions)
        .property("max_solutions",         &WasmConfig::max_solutions)
        .property("max_runtime",           &WasmConfig::max_runtime)
        .property("verbose",               &WasmConfig::verbose)
        .property("early_stopping",        &WasmConfig::early_stopping)
        .property("simplified_evaluation", &WasmConfig::simplified_evaluation)
        .property("solution_callback",     &WasmConfig::solution_callback);

    class_<WasmSolverRunner>("SolverRunner")
        .constructor<const WasmConfig&>()
        .function("run",     &WasmSolverRunner::run)
        .function("runFile", &WasmSolverRunner::run_file);
}