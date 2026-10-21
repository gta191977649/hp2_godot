# TASK-0077 P0 source review

## E-0077-P01 - Port-side benchmark export and baseline ownership

The user authorized P0 of the Fukami-inspired architecture plan. Only benchmark
and Profile tooling changes in this phase. No recovered function, native render
algorithm, physics, core compilation flag or default renderer policy changes.
Fukami source is an engineering reference, never evidence of HP2 behavior.

ProfileDebug accepts an optional held-export capture. It stops collecting at the
bounded limit, records its completion process frame and rejects manual exports
or replacement UI captures while held. The benchmark releases the hold after
the entire measured loop, including long stress loops, and records synchronous
dispatch plus asynchronous completion waits separately. Ordinary non-benchmark
capture still exports automatically. Shutdown may release the hold to preserve
data; aborted runs cannot pass benchmark validation.

The benchmark explicitly requests environment capture off by default, matching
TASK0076, with a separate opt-in pressure parameter. Scenario and snapshot replay
cannot silently restore capture. Unverified direct viewport GPU values stay raw
diagnostics at every resolution. No whole-frame GPU estimate is manufactured.

The launcher records exact engine/project/DLL/source hashes and configured build
recipe identities, then checks them again after execution. Existing output runs
are not overwritten. A copied DLL is identified by its hash; a configured recipe
is explicitly NOT proof that it produced that DLL. A/A deterministic-state
controls must pass before candidate diagnostics are evaluated. Timing statistics
are not expected to be byte-identical. Timed empty command digests remain missing
evidence, not equivalence. Detailed command/resource locations are reported only
when supplied records contain them; no native command dump is added in P0.

TASK0076 review and scope remain frozen. The successor checks the prior native
identity inventory unchanged and permits exactly four modified auxiliary files:
ProfileDebug, track_performance, run_track_costs and the predecessor's live-check
delegation. All other historical auxiliary hashes remain required. New tooling
and contract fixtures are separately pinned. Tests and measured results belong
in docs/PERFORMANCE_TASK0077.md, outside this immutable review description.
