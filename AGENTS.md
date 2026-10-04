# AGENTS.md

C++17 thermal fission gas release (TFGR) simulator. Single executable `tfgr`; one ctest
(`test_validator`). The MS-13 input data lives outside the repo (see **Data** below).

## Build & run

```bash
cmake -S . -B build && cmake --build build -j
./build/tfgr history.csv config.txt output.csv [ref_csv]
ctest --test-dir build --output-on-failure
```

CLI accepts **3 or 4** positional args. The 4th (`ref_csv`) is a Jernkvist-2019 reference
CSV (5-column `time_s,...,FGR_cumulative`) — when present, the run diffs against it after
the simulation and exits non-zero if `max_abs_error > 0.10`. Validation is only meaningful
after a LOCA-mode run (the legacy CSV produces 3-column output and is skipped by the
parser).

CMake variants (configure-time `-D...`):
- `-DDELAUNEY_MODEL=A|B` (default `A`) — exposed as `DELAUNEY_MODEL_A` / `DELAUNEY_MODEL_B`.
- `-DUSE_LIBTORCH=ON -DCMAKE_PREFIX_PATH=/opt/libtorch` — opt-in backend for the NN model
  (currently `NNModel` is implemented from scratch with `dense`/`relu`/`sigmoid`; this
  option is reserved for a future LibTorch implementation).

Release adds `-O3 -march=native`. CPack produces `TGZ;ZIP`. Install uses GNUInstallDirs
and ships `examples/history.csv` + `examples/config.txt` as `OPTIONAL` (the directory is
absent; don't add it just to silence a warning).

## Modes (selected by config keys)

**Legacy CSV** (default if none of the LOCA keys below are set): `model=Delauney|NN`,
`porosity`, `radius`, optional `nn_model=path` (required iff `model=NN`). Reads
`history.csv` (2 cols: `time_s, temperature_K`) and writes a 3-column CSV via
`ResultsWriter`.

**LOCA driver** (Task 7.3): if config additionally sets `fa_number`, `fr_input`,
`rad_fg`, `relap_plot`, the executable runs the multi-slice `runLocaDriver` flow:
1. Joins the GALILEE FR table + the rad_FG table per `(FA, Ax_slice, Rad_node)` to build
   `MicrostructureCell`s (`tfgr::Microstructure::buildMicrostructures`).
3. Steps through the RELAP5 `.5tm` plot, applying each axial slice's own temperature
   column and the Van Brutzel–Castelier EOS
   (`tfgr::vanBrutzelCastelierPressure`) to compute pore-gas pressure.
3. Aggregates per-slice increments to rod-level FGR via slice-mass-weighted averaging.
4. Writes a 5-column CSV: `time_s,T_clad_K,P_gap_MPa,tFGR_step,FGR_cumulative`.

The legacy 3-arg `computeIncrement(dt, T_begin, T_end)` and the new 6-arg
`computeIncrementLOCA(dt, T_begin, T_end, pore_pressure_Pa, porosity, pore_radius_um)`
both live on `BaseTFGRModel`. The CSV path uses the first; the LOCA driver uses the second.

## What's still stubbed

- **Both ROMs return 0.0 in `computeIncrementLOCA`** (`DelauneyModel` and `NNModel`).
  The physics call path (driver → EOS → ROM → per-slice accumulation → rod-level
  aggregation → CSV) is fully wired and exercised end-to-end on real MS-19 inputs; only
  the surrogate query itself is missing. When the **MMM database** (sample points in
  `(r, α, P_norm, FGR)` space) lands:
  - `DelauneyModel::computeIncrementLOCA` body becomes a tetrahedron interpolation on
    the database.
  - `NNModel::computeIncrementLOCA` body uncomments the already-implemented
    `dense → relu → dense → sigmoid` forward pass (gated today behind the same
    `TODO(mmm_db)`).
- `NNModel::loadWeights` parses `# NNModel 3-100-1` + 501 numeric lines
  (300 W, 100 b, 100 W, 1 b). Missing/empty file → zero-init placeholder. Wrong line
  count → `exit(1)`.
- `Microstructure.{hxx,cxx}` defaults — `defaultHbsFraction`, `defaultPoreRadius_m`,
  `defaultPorosity` — are linear / constant placeholders pending the Cappia 2016 +
  Barani 2020 publication-grade fits (each carries a `// TODO(...)` marker).

## Conventions that bite

- **Spelling is "Delauney" everywhere** — header, source, CMake option, config key, error
  message, parser comparison. Don't "correct" half of them: `parseConfig` does an exact
  string compare on `cfg.model_name`.
- **Errors go to `std::cout` then `exit(1)`**, not `std::cerr`. Match the existing style
  in `InputReader.cxx` / `ResultsWriter.hxx` / `Microstructure.cxx` / `Eos.cxx` /
  `Driver.cxx` / `Validator.cxx` / `FRInput.cxx` etc.
- **`-Wconversion -Wshadow -Wextra -Wpedantic` are on**.
  - `-Wshadow` interacts with `BaseTFGRModel`'s public `porosity` / `radius_um` data
    members: subclass overrides of `computeIncrementLOCA(..., porosity, pore_radius_um)`
    name those parameters `/*porosity*/` and `pore_radius_um` (comment form in the
    `.hxx` declaration; `local_porosity` in the `.cxx` definition) to avoid the shadowing.
  - `-Wconversion` rejects any implicit int↔double arithmetic — `static_cast` explicitly.
- `fatal_error` / `exit(1)` for unrecoverable input errors; never `std::cerr` for them.
  The only `std::cerr` use is the usage line in `main` and the per-step error inside
  `runLegacyCsvFlow` (legacy exception path).

## Data (not in repo, MS-13 only)

The LOCA flow reads 4 file types from wherever the `fr_input` / `rad_fg` / `relap_plot`
config keys point. Typical MS-13 layout (used in the end-to-end smoke):

- `OperaHPC_LOCA_Input/VVER_Temelin_18M_Standard-clad_FR_input_LOCA_{BOC,MOC,EOC}_Ax{10,22}.dat`
  — 2-line header then per-row (FA, Ax_slice, … 22 floats). Loader: `parseFRInput`.
- `OperaHPC_LOCA_Input/PowerHistories/Axial_Resolution_{10,22}/VVER_Temelin_18M_FA__NNNNN.ph`
  — 3-line header then per-step (`time_efph, burnup[N], lhgr[N], fnflux[N]`); N=10 or
  22 auto-detected. Loader: `parsePowerHistory`.
- `OperaHPC_LOCA_Input_rad_FG/VVER_Temelin_18M_Standard-clad_rad_FG_LOCA_{BOC,MOC,EOC}_Ax22.dat`
  — 2-line header then per-cell (FA, Ax_slice, Rad_node, rel_rad, burnup, gas_c, gas_s).
  Loader: `parseRadFG`.
- `VVER_LOCA_RELAP_Transient_Results/5tm.NNN` — RELAP5 plot file; 4 quoted-token header
  rows then unquoted numerics; columns are `time_run, time_s, 22 Tfuel, BALAST, 22 Tfuel,
  BALAST, 22 Pgap`. The two Tfuel blocks are surface and centre; the driver uses surface.
  Loader: `parseRelapPlot`.

## Layout

- Headers `include/tfgr/*.hxx`, sources `src/*.cxx`, tests `tests/*.cxx` — match the
  extensions when adding files.
- One ctest target: `test_validator`. Tests do **not** need network or external services;
  they parse + diff against user-supplied CSVs (`<our_csv> <ref_csv> <threshold>`).
- `build/` and `data/` are gitignored. `data/` holds MS-13 inputs only — never commit
  contents under `data/`.