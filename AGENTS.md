# AGENTS.md

C++17 thermal fission gas release (TFGR) simulator. Single executable `tfgr`. No test suite, no CI.

## Build & run

```bash
cmake -S . -B build && cmake --build build -j
./build/tfgr history.csv config.txt output.csv   # exactly 3 positional args
```

Variants (`-D...` at configure time):
- `-DDELAUNEY_MODEL=A|B` (default `A`) — exposed to code as `DELAUNEY_MODEL_A` / `DELAUNEY_MODEL_B`.
- `-DUSE_LIBTORCH=ON -DCMAKE_PREFIX_PATH=/opt/libtorch` — opt-in backend for the NN model.

Release builds add `-O3 -march=native`. CPack produces `TGZ;ZIP`. Install uses GNUInstallDirs and
installs `examples/history.csv` + `examples/config.txt` as `OPTIONAL` (the directory is absent;
don't add it just to silence a warning).

## What's still stubbed

- `src/DelauneyModel.cxx::computeIncrement` returns `0.0` — the real FGR physics is the TODO, not a finished impl.
- `model=NN` is accepted by the config parser, but only "Delauney" is wired in `src/Main.cxx`,
  `src/NNModel.cxx` is **commented out** in `CMakeLists.txt`, and `include/tfgr/NNModel.hxx` does not exist. Touching NN means uncommenting CMake, adding both files, and extending the `if (cfg.model_name == ...)` switch in `Main.cxx`.

## Conventions that bite

- **Spelling is "Delauney" everywhere** — header, source, CMake option, config key, error message. Don't "correct" half of them: `parseConfig` does an exact string compare on `cfg.model_name`, and the CMake cache property `STRINGS` would also reject `Delaunay`.
- **Errors go to `std::cout` then `exit(1)`**, not `std::cerr`. That's the existing style in `InputReader.cxx` and `ResultsWriter.hxx` — match it instead of introducing a second convention.
- **`-Wconversion -Wshadow -Wextra -Wpedantic` are on**. `-Wconversion` is the one that fails review; any int↔double arithmetic in the parser / writers needs an explicit `static_cast`.

## Input formats

- **Config** (`key=value`, one per line; `#` starts a comment; CRLF tolerated). Required: `model` (`Delauney` or `NN`), `porosity` (must parse to `[0,1]`), `radius`. `nn_model` is mandatory iff `model=NN`.
- **CSV** (two columns: `time_s, temperature_K`; `#` comments; optional header row skipped by detecting a non-numeric first cell; rows **auto-sorted by time** on load).

## Layout

- Headers `include/tfgr/*.hxx`, sources `src/*.cxx` — match the extensions when adding files.
- `ResultsWriter` writes the CSV header (`time (s),temperature (K),FGR (/)`) in its constructor and is non-copyable.