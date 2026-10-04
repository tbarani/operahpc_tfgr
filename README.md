# operahpc_tfgr

Standalone C++17 fuel-performance code for the OperaHPC project: simulates fission gas
release (FGR) during a LOCA on a VVER-1000 fuel rod, using the multi-slice surrogate
reduced-order models developed in WP6 (CEA Task 6.2) and the simulation strategy of
Task 7.3.

## Build

```bash
cmake -S . -B build && cmake --build build -j
```

Requires CMake ≥ 3.16 and a C++17 compiler. No external dependencies — the project is
deliberately stdlib-only.

## Run

The executable `tfgr` has two operating modes, selected by the keys present in
`config.txt`:

### Legacy CSV mode (default)

Two-way CSV loop driven by a single (time, temperature) trajectory.

```bash
./build/tfgr history.csv config.txt output.csv
```

`config.txt` requires `model=Delauney|NN`, `porosity`, `radius`; `nn_model=...` is
required when `model=NN`.

### LOCA driver mode (Task 7.3)

Multi-slice rod-level FGR driven by the GALILEE radial outputs and a RELAP5 thermal
transient. Set these extra keys in `config.txt` to enable:

| Key          | What it points at                                    |
|--------------|------------------------------------------------------|
| `fa_number`  | Fuel assembly to simulate, e.g. `_00415`               |
| `fr_input`   | GALILEE FR input `.dat` for that assembly            |
| `rad_fg`     | Matching `*_rad_FG_*.dat`                            |
| `relap_plot` | RELAP5 `.5tm` plot file (e.g. `5tm.491`)             |

The driver integrates each axial slice independently against its own RELAP5 temperature
column, computes the pore-gas pressure with the Van Brutzel–Castelier EOS, and
aggregates per-slice increments to rod-level FGR via slice-mass-weighted averaging.
The output CSV has columns `time_s,T_clad_K,P_gap_MPa,tFGR_step,FGR_cumulative`.

A complete example `config.txt`:

```
model=Delauney
porosity=0.05
radius=0.5
fa_number=_00415
fr_input=/path/to/VVER_Temelin_18M_Standard-clad_FR_input_LOCA_EOC_Ax22.dat
rad_fg=/path/to/VVER_Temelin_18M_Standard-clad_rad_FG_LOCA_EOC_Ax22.dat
relap_plot=/path/to/5tm.491
```

### Validation hook (Jernkvist-2019)

Append a fourth positional argument pointing at a 5-column reference CSV to diff
against after a LOCA run; the executable exits non-zero if `max_abs_error > 0.10`:

```bash
./build/tfgr history.csv config.txt output.csv /path/to/jernkvist_ref.csv
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

A single ctest target, `test_validator`, exercises the Jernkvist diff path. Tests do
not require network or external services.

## Status

Both surrogate models — `DelauneyModel` (Delaunay triangulation, WP6) and `NNModel`
(3-100-1 MLP) — currently return `0.0` from their `computeIncrementLOCA` body. The
full physics call path (driver → EOS → ROM → accumulation → CSV) is wired and
exercised end-to-end on real MS-13 inputs; the surrogate query itself is gated on
the **MMM surrogate training database** (sample points in `(r, α, P_norm, FGR)`
space) being delivered. The drop-in points for that data are:

- `DelauneyModel::computeIncrementLOCA` in `src/DelauneyModel.cxx`
- `NNModel::computeIncrementLOCA` in `src/NNModel.cxx` (forward pass already
  implemented; only the final `return` is gated)

## License

LGPL v3 — see [LICENSE](LICENSE).

## References

- I. Ramière, T. Barani, B. Michel — *Surrogate models for fuel overfragmentation
  based on MMM calculations*, OperaHPC WP6 (2026).
- B. Michel, T. Barani, I. Ramière — *Fission gas release and fuel fragmentation in
  HBS microstructure during a LOCA in a VVER*, OperaHPC WP7.3 (2026).
- L. Van Brutzel, A. Castelier — *Atomistic modelling of fission gas clustering in
  UO₂*, J. Nucl. Mater. **352** (2006) 93–110.

## For AI coding agents

See [AGENTS.md](AGENTS.md) for the conventions, gotchas, and exact interfaces an
agent must respect when editing this repo.