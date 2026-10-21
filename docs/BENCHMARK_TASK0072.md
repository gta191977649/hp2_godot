# TASK-0072 native-resolution benchmark

The existing benchmark accepts explicit -Width2880 -Height1695 without changing
TASK-0071 defaults (1920x1080, legacy deterministic loop,120warm/600samples/3repeats).
Run only one graphical workload at a time, using an isolated project and DLL.
No measured performance or PS2 equivalence is established by these harness changes.

## Camera and routes

- -Pose user_capture places the camera at user HP2 coordinates
  (-238.34,-455.37,-34.04), host YZX(-455.37,-34.04,-238.34).
- Only position is known from the screenshot. Heading0 faces native+X;90 faces+Y;
  180 faces-X;270 faces-Y. Horizontal orientations are synthetic port-side probes.
  -Headings0,90,180,270 runs separate sequential processes/documents. A dense
  result is measured, not assumed from this pose's name.
- -Route fixed holds camera transform; dense sweeps yaw by25degrees sinusoidally
  at the same position; short uses the existing6-unit lateral/12-unit forward
  closed camera loop. Warmup and measurement cover the same path. Physics stays
  paused and visual simulation advances1/60 per frame.
- -Snapshot restores a full known camera basis/effect configuration instead.
  It takes precedence over the pose and heading. Multiple headings with a
  snapshot are rejected, since they would duplicate the same restored basis.
- Full schema1 camera/effect/viewport/native-selection snapshots are exported
  after warmup and before timing in every repeat, including synthetic provenance.
  Transient particles/reflection history are not a deterministic replay file.

## Reproduce

Run each route with the same isolated project, Godot/settings/UI state and flags
for baseline and candidate, changing only the selected optimization flags:

```powershell
.\tools\run_track_costs.ps1 -ProjectPath .\local\task0072\candidate\godot -OutputDirectory .\local\task0072\measurements\candidate-fixed -Tracks 11 -Context viewer -Car CORVETTE -Pose user_capture -Route fixed -Headings 0,90,180,270 -Width 2880 -Height 1695 -WarmFrames 120 -Samples 600 -Repeats 3 -ProfileUi
```

VSync is disabled and max_fps=0. Minimized windows invalidate a run. Profile UI
uses the existing native panel and DebugTools startup; layout is controlled only
by keeping the same project/settings and UI startup across pairs. This harness
does not add an ImGui layout lock or claim to reproduce the screenshot layout.
Native runs verify actual client viewport size and reject a constrained
2880x1695 window before measurements.

## Interpretation and compatibility

One output document per heading/route preserves TASK-0071 comparison keys.
The TASK-0071 comparator remains strict1080p and must not be used to certify a
native-resolution run. Native results include native_resolution_protocol for
parameter/backend/build checks; it is not a completed acceptance verdict.
Each row includes frame_budget_met only when sampling completed and wall median
andP95 are both <=1000/60ms. ReportP99 and repeated-run noise too, and require
packet/pixel/depth parity separately. No screenshot or private data is tracked.

TASK-0072 direct viewport query CPU/GPU metrics are unavailable without source
frame provenance; raw_unverified values are retained for diagnostics only. They
are never included in accepted CPU/GPU summaries. ProfileDebug exports retain
their separately validated timestamp source. wall_ms remains the primary metric.
TASK-0071 legacy delayed root readings and existing acceptance_protocol behavior
are unchanged.

## Validation

PowerShell AST syntax parsed without errors. Godot4.7.2 headless --check-only for
res://scripts/runtime/track_performance.gd passed, exit0. No graphical workload,
GPU timing or build was run by this harness change.

## Actual TrackViewer context

-Context viewer uses track_performance_viewer.gd, a subclass of the actual
asset_viewer.gd. It invokes the production startup, TrackViewer UI registration,
load dispatcher, publish/refresh methods and exactly one AssetViewer._process
per sampled frame. Its car is hidden but retains the production native car
update. Free browsing supplies mover2 and visual_state{}, so no live-car
reflection origin is introduced. No effect is disabled to manufacture that state.
The requested CORVETTE is selected through the production select_car action.
The wrapper replaces only the automatic first-track startup request and deferred
first-car selection with explicit benchmark selections; --viewer-smoke is rejected
because it would install a separate smoke runner.

-Context race remains the default and retains paused race runtime plus its live
vehicle environment capture. Those earlier samples are labeled race; they are
not comparable to the screenshot's free-browse context. Context and selected car
are exported in run metadata, snapshots and each result. Per-sample viewer
process-update counters expose accidental multiple process calls. Viewer defaults
to CORVETTE if -Car was omitted; race keeps ELISE. Native protocol eligibility
requires viewer context, while the TASK0071 legacy flag remains race-only.

The inherited wrapper cannot be independently --check-only compiled without
project autoload global registration. A headless loader with actual project
autoloads compiled it successfully (TASK0072_VIEWER_SCRIPT_COMPILE_OK); it did
not instantiate the viewer or launch a GPU workload. Main harness check-only,
PowerShell parsing and diff whitespace checks also pass. Actual viewer loading,
full UI layout and rendering remain root-owned GPU validation.

## Accepted-series input isolation and reporting

Final v3 measurements disable external camera input and use a distinct project
application name to isolate ImGui/user settings. A valid run exports
external_camera_input=false and passes exact paired pose identity. v1/v2 captures
with script errors, incomplete runs or changed poses remain excluded. Heading180
is the densest exploratory direction at the supplied position, not a recovered
screenshot basis. Do not replace the unknown basis with an original-camera claim.

The native comparator's --optimization option accepts sun(default), compact, bulk
or combined and requires exactly the expected boolean false->true flag set.
Protocol validity, measured benefit beyond baseline repeat noise and all-repeat
60FPS are separate verdicts. Main-thread profile export spikes remain included;
a median/P95 pass does not promise every frame meets budget. Read final results in
[performance report](PERFORMANCE_TASK0072.md), whose bulk timing section is pending
at this checkpoint. Legacy race results are not free-browse screenshot results.

## Simultaneous movement and rotation

-Route short_spin traverses the existing synthetic short camera path while yaw rotates through720 degrees. Warmup and sampling cover the same complete trajectory. Use fixed/dense/short/short_spin to separate static cost, angular changes, translation and their combination. This fixture is free browsing or paused race according to Context, never actual vehicle driving. Actual Drive mode must be tested separately with native physics and deterministic vehicle input.

## Actual Test drive and continuous stress harness

-Context driving uses the same AssetViewer subclass, calls the production
Test drive action and driving_lab, and leaves the physical simulation running.
It does not use the paused race benchmark. -Route drive supplies semantic held
throttle0.35, brake0, steering0, handbrake0 through driving_input. -Route drive_spin
also sets the existing port driving camera yaw to PI*sin(frame*elapsed*TAU/4)
and resets mouse_idle before the production camera update. This changes only
port camera look direction; it is not a reconstructed original camera algorithm.

Each frame calls the actual AssetViewer._process exactly once with elapsed1/60.
The native reported dt is retained as evidence, including any original timestep
quantization. The harness requires exactly one native tick invocation, permits original step
delta0or1 (minimum_frame_elapsed can hold a step), and requires known
body/wheels and a valid body/dt, plus unchanged semantic inputs. Failure pauses
and errors; it never silently reinitializes physics or teleports onto a route.
The initial production Test drive initialization step precedes warmup. Driving
continues across the120warm/600measured boundary with no reset of vehicle,
input or camera phase. Each repeat creates a fresh viewer/native driving session.

Only the benchmark wrapper forces its driving focus flag and ignores external
focus/control events; production lifecycle code is unchanged. It disables external
DebugTools input while preserving its actual UI process. Cursor capture is not
requested by this synthetic control harness. A paused/failed driving lifecycle,
changed input, hidden driving car, minimized window or invalid native step fails
the run. The initial conservative straight-throttle route is not GPU-validated
until an actual successful run and paired checks are recorded; it is not an
assertion that an arbitrary long straight input follows Track11 safely.

Driving requires -Pose start, no snapshot/fixed-camera/heading overrides, and
uses CORVETTE by default. Run each baseline/candidate in a distinct output:

```powershell
.\tools\run_track_costs.ps1 -ProjectPath .\local\task0072\candidate\godot -OutputDirectory .\local\task0072\measurements\drive-baseline -Context driving -Car CORVETTE -Tracks 11 -Route drive -Width 2880 -Height 1695 -WarmFrames 120 -Samples 600 -Repeats 3 -ProfileUi
```

The row's driving_timeline contains all warm/measured step, body position/bits,
camera position/bits, yaw, input and native dt evidence. warm_end_step anchors
the boundary; raw measured samples also include their driving record. Exact
physical/body/camera evidence must agree across paired runs. Driving is excluded
from legacy acceptance_protocol and viewer-only native_resolution_protocol.
driving_resolution_protocol describes the requested native Debug/Profile matrix;
it does not by itself certify physics, visual parity or60FPS success.

All benchmark contexts additionally record before/after/delta values for the five
pinned Godot4.7.2 Performance pipeline-compilation monitors (canvas, mesh, surface,
draw, specialization). Missing/reset counters report null deltas, not fabricated
savings. Post-draw evidence collection stays outside the wall timer as with prior
counter collection. Source data is not a summed GPU rendering-time measurement.

For a wall-time stress run, add -DurationSeconds 300 -Repeats 1. The driving loop
collects at least -Samples measured frames AND at least300seconds of post-warm
wall time, with unlimited FPS. It keeps compact per-frame counters/costs/driving
evidence; large repeated resource/viewport/submission dictionaries are omitted,
and the separate timeline retains warmup only since measured driving evidence is
already in samples. The normal profiler captures the first requested sample_count
frames; its UI remains live afterwards. Such a run is long_run_stress, requires
long_run_complete, and never claims the strict600sample protocol. No sync export
spikes are removed from the first capture or wall-time stress duration.

-Route rotate remains a viewer-only synthetic in-place720degree yaw probe;
short_spin traverses the closed short route while turning720degrees. They now
have explicit viewer protocol eligibility; neither substitutes for actual driving.
Launcher candidate flags also accept independent depth-discovery-cache,
dynamic-plan-cache and profile-async-export toggles for attribution measurements.

Validation performed for the harness change: main GDScript check-only passed;
headless task0072_driving_harness_smoke.gd passed semantic-input, step/lifecycle/
body/dt/input-contamination and pipeline-reset contracts without instantiating
physics or a GPU viewer. PowerShell AST and whitespace checks passed. The root
runner owns the actual GPU driving/route validation.

Async profile exporters are polled after timed sampling and end-of-route captures,
with a30second deadline and process-frame yielding. The harness never calls the
blocking shutdown drain inside a benchmark. Each result records exported/pending/
status/error and untimed wait duration; failure or timeout invalidates the result.
The ordinary sample-limit export request still occurs in the actual production
profiler process, so snapshot cloning and other main-thread work remain included
in measured frames. Completion waits do not hide synchronous baseline export work.

Per-frame driving evidence also retains each wheel's optional contact/grip terrain
and native speed/forward-speed telemetry. Missing contact/terrain remains null.
The result summarizes position bounds, speed range, contact-count histogram,
zero-known-contact frames, unknown-contact frames and sampled terrain IDs across
both warmup and measured frames, including long runs. These fields help detect a
straight-throttle run leaving the road, but do not provide a route-membership
oracle: a finite body or a completed wall-time soak alone never proves a valid
Track11 driving route. A route that leaves its intended road remains a blocker
and must not be reported as validated driving/soak acceptance.

Driving pilot counterevidence: an earlier wrapper allowed inherited non-focus
notifications to run driving.sync_lifecycle asynchronously; a pilot stopped
before600samples, but the exact suspension trigger was not established. Such
incomplete results are invalid. The wrapper now isolates
all inherited notification lifecycle dispatch while forcing only its own focus;
the actual driving process still checks lifecycle once per explicit step. Rejected
steps report exact phase/active/error/focus/console/keyboard-capture and native
step/status/paused diagnostics. No suspended/error run is silently resumed.

Initial-clock correction: another driving pilot rejected a first1->1 native step
with no samples. At successful Test drive startup only, the wrapper now records
and clears the production discard_elapsed latch before beginning its fixed
benchmark clock. driving_startup exports the before/after latch, initial step and
phase. No warm/measured frame is skipped, and subsequent lifecycle/discard/step
failures still reject immediately rather than being reset or silently resumed.
The earlier incomplete series remains excluded.

Scheduler counterevidence supersedes the earlier exact-step-advance assumption:
Session::tick preserves minimum_frame_elapsed and may validly return the previous
committed body/step after one real tick call. A zero step delta with valid native
state is therefore accepted, while negative or multiple-step deltas are rejected.
A port-only driving_lab.native_tick_calls counter increments immediately before
both startup and process lab.tick calls, leaving behavior/order unchanged. Each
benchmark frame exports its call-counter endpoints, exactly-one invocation delta,
actual native step_delta0or1 and elapsed-discard evidence. Missing/multiple tick
calls and lifecycle discards still invalidate the run; no extra ticking, elapsed
substitution or reset is introduced. The previous startup1->1 rejection had a
false discard latch and was not evidence of a lifecycle fault. The startup-only
latch normalization did not explain that legitimate hold. Earlier complete series
whose steps all advanced remain a valid subset of the corrected scheduler contract.
Headless contracts cover accepted holds and rejected missing/multiple tick calls.
