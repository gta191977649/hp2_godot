# Feasibility and workload estimate

Assessment date: 2026-10-10. Scope: native Windows port preserving original
gameplay, using C++ core/platform contracts and Godot as host. No game
implementation or timed reconstruction pilot was executed.

## Feasibility conclusion

The project is technically plausible enough to justify a bounded pilot, but a
full-game completion commitment is not supported yet. The native/Godot scaffold
already passed its infrastructure smoke test. The supplied EE ELF is accessible,
R5900 analysis/MCP queries work, and diagnostic strings provide useful anchors.
These facts support starting analysis; they do not establish recoverable coverage
or original-behavior equivalence.

The main risks are function/ABI quality, COP2/VU/IOP behavior, unknown state/data
layouts, asset/render adaptation and the missing original-execution oracle.
7,544 function entries are **not** 7,544 required manual rewrites. Some entries
may be SDK/runtime stubs, thunks, duplicates, unused paths or incorrect boundaries.
Conversely, undiscovered code, VU programs, overlays and IOP behavior can add work.

## Observed basis and what remains unmeasured

Observed: 7,544 entries, 17,162 call pairs, 400,582 decoded `.text` instructions,
663 jalr sites, a complexity tail reaching 233, 26 empty CFGs, 21 available probe
outputs, and ~3.10 GB in 402 private asset files. See BINARY_REPORT.md for scope.

Unmeasured: manual-recovery fraction, engineering hours per function/layout/format,
PCSX2 comparison cost, number of required VU/IOP programs, reusable library matches,
feature completeness/fidelity target, and team throughput. Actual pilot recovery,
implementation and verification engineering times are **not measured**, not zero.
Tool-call duration and AI token consumption are not converted into engineering hours.

## Model

For phase p and complexity stratum s:

`H_p = sum(N_s * q_s * r_s,p) + shared_phase_work_p`

N is the measured entry population. q is an assumed fraction requiring distinct
manual behavioral work, after SDK/thunk/duplicate/unused handling. r is engineering
hours per retained function-equivalent for that phase. Shared layout/format/adapter
work is budgeted once, not charged again to every caller. An additional integration
and rework reserve applies to the combined direct work.

For this initial sensitivity calculation, q is uniformly 0.20 / 0.40 / 0.70
for optimistic / baseline / pessimistic scenarios. These fractions are **unverified
assumptions**, not measured SDK exclusion rates or probabilities. Uniform retention
is a simplification; a calibrated model should use subsystem/complexity-specific q
and include newly discovered EE/VU/IOP work explicitly.

Rates below are planning assumptions in person-hours, shown as RE / C++ / testing
and comparison. They are not benchmarks or forecasts from the 21 probes.

| CFG stratum | Observed N | Optimistic rates | Baseline rates | Pessimistic rates |
|---|---:|---|---|---|
| Low <=5 | 6,103 | 1 / 0.5 / 0.75 | 3 / 2 / 2 | 8 / 6 / 6 |
| Medium 6–20 | 1,224 | 4 / 3 / 4 | 12 / 10 / 12 | 32 / 24 / 40 |
| High 21–60 | 173 | 16 / 12 / 24 | 48 / 40 / 72 | 120 / 100 / 200 |
| Very high >60 | 18 | 40 / 32 / 60 | 120 / 100 / 180 | 300 / 240 / 500 |
| Unknown CFG | 26 | 8 / 4 / 8 | 40 / 24 / 48 | 160 / 80 / 160 |

The hardware/dependency/layout axes are not collapsed into invented multipliers.
They inform broad scenario rates, separate shared budgets and the rework reserve;
after pilots they should become measured strata. A simple syscall can have low
CFG complexity but a costly host contract. These preliminary numbers can therefore
miss work in both directions and must be recalibrated.

## Scenario results

Rounded to avoid implying calibrated precision. Exact arithmetic and assumptions
are in ESTIMATE_SCENARIOS.json; values below are scenario outputs, **not an
evidence-supported delivery schedule**.

| Work category | Optimistic hours | Baseline hours | Pessimistic hours |
|---|---:|---:|---:|
| Reverse engineering | 2,940 | 17,800 | 82,820 |
| Core C++ reconstruction | 1,900 | 13,520 | 62,790 |
| Data structure recovery, shared | 400 | 2,000 | 8,000 |
| Resource extraction/conversion | 400 | 1,500 | 6,000 |
| Rendering/VU adaptation | 1,000 | 4,000 | 15,000 |
| Audio/IOP adaptation | 250 | 1,000 | 4,000 |
| Godot integration | 200 | 800 | 2,400 |
| Verification/testing | 2,980 | 17,540 | 93,340 |
| Direct subtotal | 10,070 | 58,150 | 274,340 |
| Integration/rework reserve | 15% | 35% | 75% |
| Total scenario envelope | **~12,000** | **~79,000** | **~480,000** |

Interpretation: full-game fidelity could be a very large, multi-year/multi-person
effort. The numerical spread mainly reflects unknown retention and rates; it is
not a confidence interval. Baseline does not mean most likely. The upper scenario
is a stress case, not an upper bound: major undiscovered code or stricter fidelity
could exceed it. A reduced vertical slice could be far smaller but needs its own
explicit dependency/asset scope and estimate.

Changing baseline q by 0.10 changes function-phase direct work by roughly 12,210
hours, before reserve. Adapter/shared work is a separate uncertainty. This
sensitivity is why calibration is more valuable than refining totals now.

## Pilot and calibration plan

The 20-function pilot is intentionally selected across simple helpers, high-fan-in
services, hardware, AI/physics/state, large unknown code and damaged metadata.
It is a purposeful exploratory sample, not an unbiased random estimate of the
whole population. Do not multiply its average time by 7,544.

First establish a comparison harness and execute one small behavior slice
(`P010`, byte fill). Illustrative budget: RE 2/6/16 h, C++ 2/4/8 h, comparisons
4/12/32 h; shared reference setup 16/40/80 h. Total first-slice scenario:
24/62/136 h. These assumptions are not observed throughput. Stop and revise
the method if original invocation/capture cannot be made reproducible.

Then measure the varied pilot functions (per-function scenario budgets are in
PILOT_TASKS.md). Record analyst time, reconstruction time, layout/shared work,
adapter work, comparison time, failures and rework separately. Record discoveries
that expand scope. Use actual timed measurements within matched strata; allocate
shared work once and validate the manual-retention partition. Expand sampling
where a subsystem/difficulty stratum has no reliable coverage.

Decision gates:

1. Repair/audit bad boundaries and ABI before accepting population metrics.
2. Demonstrate repeatable original capture and at least one verified core function.
3. Demonstrate one physics/AI/state slice and one hardware-facing contract; report gaps.
4. Measure one asset format and a representative rendering/audio path.
5. Define target completeness/fidelity, dependency closures and exclusions before
   committing to M5/M6 or a full-game staffing/schedule estimate.

No full game implementation, budget commitment or successful original comparison
is claimed by this assessment.
