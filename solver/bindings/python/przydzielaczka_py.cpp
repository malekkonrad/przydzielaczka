#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <config.h>
#include <solver_runner.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <stdexcept>
#include <string>

namespace py = pybind11;

// Python-friendly config that accepts any callable for solution_callback.
struct SolverConfig {
    size_t max_keep_solutions    = 10000;
    size_t max_solutions         = 10;
    size_t max_runtime           = 10000;
    bool   verbose               = false;
    bool   early_stopping        = false;
    bool   simplified_evaluation = false;
    // Receives a Python dict (parsed from the solution JSON).
    py::object solution_callback = py::none();
};

// Wraps SolverRunner so that run() accepts a Python str or dict and returns a Python dict.
class PySolverRunner {
public:
    explicit PySolverRunner(const SolverConfig& py_cfg) {
        input_models::config cfg;
        cfg.max_keep_solutions    = py_cfg.max_keep_solutions;
        cfg.max_solutions         = py_cfg.max_solutions;
        cfg.max_runtime           = py_cfg.max_runtime;
        cfg.verbose               = py_cfg.verbose;
        cfg.early_stopping        = py_cfg.early_stopping;
        cfg.simplified_evaluation = py_cfg.simplified_evaluation;

        if (!py_cfg.solution_callback.is_none()) {
            py::object cb = py_cfg.solution_callback;
            cfg.solution_callback = [cb](const std::string& json_str) {
                py::gil_scoped_acquire gil;
                py::object json_mod = py::module_::import("json");
                py::object parsed   = json_mod.attr("loads")(json_str);
                cb(parsed);
            };
        }

        runner_ = std::make_unique<SolverRunner>(cfg);
    }

    // Accepts a JSON string or a Python dict/list.
    py::dict run(const py::object& input) {
        nlohmann::json j = to_json(input);
        return result_to_dict(runner_->run(j));
    }

    py::dict run_file(const std::string& path) {
        return result_to_dict(runner_->run(path));
    }

private:
    static nlohmann::json to_json(const py::object& input) {
        if (py::isinstance<py::str>(input))
            return nlohmann::json::parse(input.cast<std::string>());

        if (py::isinstance<py::dict>(input) || py::isinstance<py::list>(input)) {
            py::object json_mod = py::module_::import("json");
            std::string s = json_mod.attr("dumps")(input).cast<std::string>();
            return nlohmann::json::parse(s);
        }

        throw std::invalid_argument("run() expects a JSON string or a dict/list");
    }

    static py::dict result_to_dict(const nlohmann::json& result) {
        py::object json_mod = py::module_::import("json");
        return json_mod.attr("loads")(result.dump());
    }

    std::unique_ptr<SolverRunner> runner_;
};

PYBIND11_MODULE(przydzielaczka_py, m) {
    m.doc() = "Timetable constraint solver — Python bindings";

    py::class_<SolverConfig>(m, "Config")
        .def(py::init<>())
        .def_readwrite("max_keep_solutions",    &SolverConfig::max_keep_solutions)
        .def_readwrite("max_solutions",         &SolverConfig::max_solutions)
        .def_readwrite("max_runtime",           &SolverConfig::max_runtime)
        .def_readwrite("verbose",               &SolverConfig::verbose)
        .def_readwrite("early_stopping",        &SolverConfig::early_stopping)
        .def_readwrite("simplified_evaluation", &SolverConfig::simplified_evaluation)
        .def_readwrite("solution_callback",     &SolverConfig::solution_callback,
            "Callable(dict) — called once per solution found, receives the solution as a Python dict.");

    py::class_<PySolverRunner>(m, "SolverRunner")
        .def(py::init<const SolverConfig&>(), py::arg("config") = SolverConfig{})
        .def("run",      &PySolverRunner::run,      py::arg("input"),
            "Run the solver. input can be a JSON string or a Python dict. Returns a dict.")
        .def("run_file", &PySolverRunner::run_file, py::arg("path"),
            "Run the solver loading input from a JSON file at the given path. Returns a dict.");
}