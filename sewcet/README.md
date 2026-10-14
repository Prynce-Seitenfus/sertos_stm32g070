# sewcet — static WCET vs. profiler measurements

Project configuration and results for the static WCET analysis of
`sertos_stm32g070`. The tools, the Cortex-M0+ timing model and the OTAWA
toolchain live in the sewcet workspace (`C:\github\sewcet`, see its README
section 5.5); this folder holds only what is specific to this firmware.

## Run

```powershell
.\sewcet\run_wcet.ps1                     # m0plus model (default from JSON)
.\sewcet\run_wcet.ps1 -Model trivial      # fixed cycles per instruction
.\sewcet\run_wcet.ps1 -Model generic      # OTAWA etime pipeline/memory model
.\sewcet\run_wcet.ps1 -Elf .\build\Release\sertos_stm32g070.elf
```

The wrapper calls `<sewcet>\tools\run_wcet.ps1 -Config .\sewcet\wcet_targets.json`,
with `<sewcet>` = `$env:SEWCET_DIR` or `..\sewcet` next to this repository.
Inside WSL: `bash /mnt/c/github/sewcet/tools/run_wcet.sh sewcet/wcet_targets.json [--model generic]`.

## Files

| File | Purpose |
|---|---|
| `wcet_targets.json` | Target functions, ELF, capture paths, timing model, justified flow facts (loop bounds, indirect call targets), response-time bound specs |
| `wcet_measure.json` | Historical scope-based Measure config; its flow facts and captures are not valid for current firmware |
| `run_wcet.ps1` | Wrapper around the sewcet tools for this config |
| `1w.csv` | WCET per function in profiler-CSV columns (generated) |
| `comparison.csv` | Target / Renode / WCET comparison (generated) |
| `work/` | Generated `.ff`, `.osx`, hardware XML, `owcet` logs, built plugin (ignored by Git) |

Timing models and the config schema are documented in the sewcet README;
the M0+ parameters used here are in `timing.m0plus` of `wcet_targets.json`.

## Interpreting the results

Column and status legend: `<sewcet>/tools/README.md`.

- `1w.csv`: `calls` is 1 and `total/min/max/avg` all hold the WCET; a static
  analysis yields one upper bound per function.
- `comparison.csv` checks each WCET against the measured execution maximum
  on the target: `status` is `VIOLATION` when it exceeds `wcet_cycles`, and
  `wcet_over_exec_max` shows how pessimistic the bound is.
  `target_avg_cycles` and `renode_max_cycles` are informational (Renode is not
  cycle-accurate).
- The current profiler library reports ACTIVE cycles and keeps a shadow stack
  per execution context; time spent while a task is switched out is excluded.
  Its dump header records the mode. The previous WALL/ACTIVE capture pairs are
  historical; the current firmware cannot generate WALL captures.
- Current captures can check execution time: `target_active_max_cycles`
  (`captures.target_active`) must not exceed `wcet_cycles` (`VIOLATION`).
  WALL-based response-time checks for blocking calls require separate WALL
  measurements; they cannot be derived from the current ACTIVE-only dump.

  `OK_EXEC`/`OK_BOUND` mean only one check had data; `BLOCKING` means none.
- Response-time bound:
  `R = blocked_ticks * cpu_clock_hz / tick_hz + R_post`, with
  `R_post = WCET(self) + sum(count * WCET(interference)) + extra_cycles
  + sum((ceil(R_post / period_cycles) + 1) * WCET(isr))` iterated to a fixed
  point. Task bursts (higher-priority jobs, PendSV switches) are listed per
  function with a `reason`; periodic ISRs (`SysTick`, `TIM6`, `TIM1`) are the
  `interference_functions` entries with `period_cycles`. All
  `interference_functions` are analysed by OTAWA and appended to `1w.csv`
  but not compared.
- `--time-mode active` treats `--target`/`--renode` as ACTIVE captures: every
  function gets the execution check and no bound check, e.g.
  `python ..\sewcet\tools\compare_csv.py --config sewcet\wcet_targets.json
  --target 1a.csv --renode 1ra.csv --time-mode active`.
- The script exits with 1 on `VIOLATION`, `BOUND_VIOLATION`, `NO_WCET` or
  `NO_BOUND_WCET`.
- Build Debug with `.\build.bat -i` to include compiler instrumentation; the
  profiler hooks (`__cyg_profile_func_*`) are then analysed too
  (`instrumentation.include_profiler_hooks`).
- Measured cycles also include interrupt preemption (TIM6 tick, TIM1 profiler
  overflow, USART2); the WCET does not.

## After rebuilding the firmware

Loop offsets and `elf_checksum` are tied to one binary. The checked-in
`wcet_targets.json` values predate the latest profiler sync and must be refreshed
before using its WCET results. After a rebuild:

1. Run `mkff $ELF <functions...>` (with `otawa-env.sh` sourced), then
   update `elf_checksum` and the `offset` values in `wcet_targets.json`.
2. Re-check each bound against its `reason`.
3. Re-run `run_wcet.ps1`.
