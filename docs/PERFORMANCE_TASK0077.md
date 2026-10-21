# TASK-0077 P0: reproducible render baselines

## Scope and status

P0 implements export-separated benchmark collection, immutable run inputs,
explicit environment-capture policy and A/A-first diagnostics. P1 submission
boundaries, P2 source/instance lifetime and P3 build organization are deferred
until this baseline is accepted. No renderer speedup or original equivalence is
claimed. Fukami is an engineering reference, not HP2 behavior evidence.

The pre-change script inputs and all 555 game_core .cpp/.hpp byte hashes are
preserved under ignored `local/task0077/baseline`. Current native DLLs are copied
from the TASK0076 workspace output into `local/task0077/project/bin`; the workspace
DLL is never rebuilt or replaced by this task. No proprietary inputs or captures
are committed. Native settings remain Vulkan/Forward+, 0/Safe, dynamic capture off.

## Capture boundary

`ProfileDebug.begin_capture(warm, frames, hold_export=false)` retains ordinary
auto-export behavior. The benchmark passes true: collection stops at the bounded
limit and status becomes `ready`, but hashing, cloning, serialization, file IO and
worker dispatch are held until the complete measured loop finishes. Manual UI
export/re-record cannot bypass a held benchmark capture. Long runs retain at most
600 Profile samples and do not export when the first 600 frames finish.

Each benchmark row records measurement_end_usec, profile_export_started_usec,
profile_export_was_held, profile_capture_completed_process_frame,
profile_export_dispatch_ms, profile_export_wait_ms and profile_export_total_ms.
Dispatch and total overlap; they are never summed into render time. Both sync
and async failures invalidate the route. Profile-off runs produce no fake export
measurements. Captures, end-state digests and result JSON writes remain untimed.

Unverified direct viewport readings are raw diagnostics at every resolution.
No whole-frame GPU duration is available; validated per-viewport Profile queries
retain their own completed source-frame domain and remain non-additive.

## Identity and control protocol

The launcher records `identity.json` before execution and `identity-check.json`
afterward. Inputs include engine, isolated project configuration/scripts/shaders,
extension/dependency DLLs, native source, launcher/comparator, godot-cpp revision,
and available Debug configured recipes. Changed/missing/added input files reject
the run. Existing run.json files are preserved: use a new output directory.

A build recipe snapshot is not proof that it produced a copied DLL. The DLL's
exact SHA256 is authoritative here; full build/packaging attestation belongs to
P3. The manifest states this limitation explicitly. A local source capture does
not certify that an arbitrary prebuilt DLL corresponds to recovered source.

`--performance-environment-capture=false` is explicit by default. The launcher
offers `-EnvironmentCapture` only as a separate pressure fixture. Default and
non-default effect scenarios, including snapshot restoration, retain this policy;
`environment_off` explicitly disables it. Do not combine capture pressure with
the environment-off/all matrix. Actual effect permissions remain exported.

Controls use identical inputs and configuration in two independent processes.
The comparator requires source/engine/DLL equality, ordered complete samples,
actual thread/capture configuration and the export boundary before comparing
deterministic state. Timings may vary; their median/P95/P99/max and baseline
median range are reported, not treated as exact equality. Candidate diagnostics
are blocked when A/A fails; existing optimization-specific comparators still
decide candidate flag permissions and performance acceptance.

First divergence includes repeat, logical frame and view where exported. A
recursive structured comparison also locates command/resource indices when the
input records contain command detail. P0 adds no expensive timed command dump.
Empty timed packet digests are unavailable; final untimed digest and submitted
counts are bounded evidence, never image/whole-game equivalence. Pixel/depth/
reflection acceptance remains a separate existing parity fixture.

## Reproduction

Use an isolated project containing the current scripts and a matching local DLL.
Do not build, run another GPU benchmark or modify source while timing.

```powershell
py -3 -m unittest discover -s reverse/tests -p test_task0077_baseline.py
py -3 -m unittest discover -s reverse/tests -p test_task0077_source_review.py
py -3 reverse/scripts/audit_task0077_review.py
./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path local/task0077/project --script res://scripts/debug/task0077_capture_boundary_smoke.gd
./tools/run_track_costs.ps1 -ProjectPath local/task0077/project -OutputDirectory local/task0077/example-reference -Context driving -Route drive_spin -Height 1695 -Width 2880 -WarmFrames 120 -Samples 600 -Repeats 3 -ProfileUi
./tools/run_track_costs.ps1 -ProjectPath local/task0077/project -OutputDirectory local/task0077/example-control -Context driving -Route drive_spin -Height 1695 -Width 2880 -WarmFrames 120 -Samples 600 -Repeats 3 -ProfileUi
py -3 tools/task0077_baseline.py compare local/task0077/example-reference/run.json local/task0077/example-control/run.json --output local/task0077/example-comparison.json
```

For moving viewer rotation use `-Context viewer -Route short_spin`. Omit
`-ProfileUi` for the separate Profile-off controls. Add `-EnvironmentCapture` for
explicit capture pressure; compare it only against an equally configured A/A
control. `--diagnostic` permits reduced sampling/resolution in the comparator
but never turns a short run into strict acceptance. Retain failed raw results.

## Validation results

Tools detected: Python3.14.4, CMake3.29.2, Ninja1.12.0, Godot4.7.2 official
ed1daf0bf; pinned godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b.

- Asset-free baseline tests: 12 passed initially.
- Auxiliary source contract tests: 5 passed initially.
- Strict native identity successor: passed, zero native identity changes,
  four modified and six added auxiliary paths; TASK0076 review/scope unchanged.
- Isolated headless capture-boundary fixture: passed held limit, manual UI
  rejection, untimed synchronous export, and ordinary automatic export.
- Isolated benchmark parse check: passed after correcting an indentation error.
- Driving short Profile-on fixture: 3 warm/10 samples/one repeat at1080p passed;
  Profile10/10, held export dispatched after measurement, about365ms dispatch
  excluded from measured frames, actual environment permission off.
- Explicit capture short fixture: completed successfully; actual effect and
  final control results are recorded in the final validation checkpoint below.
- Strict driving/viewer Profile-on/off controls: in progress; no result claimed yet.

## Continuation

Finish the sequential matrix and A/A comparisons, retain raw records under
`local/task0077/`, rerun focused contracts/source checks after settling changes,
and update this report, the task, log and generated tracker. Do not start P1/P2/P3
or enable rejected TASK0072 candidates on preliminary timing alone.
