/* global self */

// Emscripten modules reference `window` — polyfill for Worker context
self.window = self;

var _modulePromise = null;

function loadModule(wasmJsUrl) {
  if (_modulePromise) return _modulePromise;
  _modulePromise = new Promise(function (resolve, reject) {
    try {
      importScripts(wasmJsUrl);
    } catch (e) {
      _modulePromise = null;
      reject(new Error('importScripts failed: ' + (e.message || e)));
      return;
    }
    if (typeof self.createPrzydzielaczkaModule !== 'function') {
      _modulePromise = null;
      reject(new Error('createPrzydzielaczkaModule not found after importScripts'));
      return;
    }
    // Emscripten can't use document.currentScript in a Worker to locate .wasm —
    // pass the directory explicitly so it fetches from the right URL.
    var wasmDir = wasmJsUrl.substring(0, wasmJsUrl.lastIndexOf('/') + 1);
    self.createPrzydzielaczkaModule({
      locateFile: function (filename) { return wasmDir + filename; },
    }).then(resolve).catch(function (e) {
      _modulePromise = null;
      reject(e);
    });
  });
  return _modulePromise;
}

self.onmessage = async function (e) {
  var data = e.data;
  if (data.type !== 'run') return;
  var id = data.id;

  try {
    var Module = await loadModule(data.wasmJsUrl);

    var cfg = new Module.Config();
    cfg.max_solutions = data.config.max_solutions;
    cfg.max_runtime = data.config.max_runtime;
    cfg.early_stopping = data.config.early_stopping;
    cfg.verbose = data.config.verbose || false;
    cfg.max_keep_solutions = data.config.max_solutions;
    cfg.simplified_evaluation = false;
    cfg.solution_callback = function (jsonStr) {
      try {
        var partial = JSON.parse(jsonStr);
        self.postMessage({ type: 'solution', id: id, partial: partial });
      } catch (_) {}
    };

    var runner = new Module.SolverRunner(cfg);
    var resultJson;
    try {
      resultJson = runner.run(JSON.stringify(data.input));
    } finally {
      runner.delete();
      cfg.delete();
    }

    self.postMessage({ type: 'done', id: id, resultJson: resultJson });
  } catch (err) {
    self.postMessage({
      type: 'error',
      id: id,
      message: err instanceof Error ? err.message : String(err),
    });
  }
};