# TASK-0075 strict source successor

## E-0075-R01: port-side storage and host presentation

This is a source-change scope record, not original-game behavior evidence.
The predecessor is the frozen TASK0072 approved source review and scope. Their
normalized LF SHA-256 identities remain pinned by audit_task0075_review.py.
Earlier frozen reviews are validated recursively against their explicit historical
identity and auxiliary snapshots. Only the no-argument TASK0072 live entrypoint
delegates to TASK0075; historical validation remains unchanged.

The successor permits explicit wheel-effect producer and dispatch slots under
E-0075-T01/C01, world-update scheduling under E-0075-P03, and explicitly named
host/resource/dynamic-storage slots under this record. It preserves exact function
paths, names, signatures and body hashes, duplicate counts, stub classifications,
auxiliary source fingerprints and evidence annotations. Unlisted changes fail.
Car::TriggerCarTerrainEffects remains a partial boundary with an explicit throw
for its remaining branches. Its inventory parser currently classifies the wrapper
as nonstub; that classification must not imply whole-entry implementation.
The MotionLive current-car velocity correction in the particle update adapter
and its setter are separately anchored to E-0075-D02 in dynamic-particles.md;
static groups with null pCar use zero current-car velocity.

The port-owned additions connect shared terrain emitter copies to existing
original particle generation and rendering. Godot objects display generated
particle output; they supply no replacement driving physics or invented particle
curves. Render resources, display lifecycle, simulation replacement, dynamic
emitter indexing and explicit test-drive distance policy are host concerns.
Original behavior statements require the independent function records.

## Pre-existing source changes kept separate

TASK0074 wireframe controls were already present in asset_viewer.gd when TASK0075
started. That file also changes for the default browsing projection and wheel
particle host connection. Its whole-file auxiliary pin necessarily covers both;
wireframe is a prior host change, never wheel-physics evidence.

build.ps1 already differed from the TASK0072 auxiliary pin at startup. TASK0075
does not modify it. The successor records the existing working-tree build helper
as preserved prior state, not a TASK0075 implementation. Its profiler, worker and
Godot-version changes belong to the earlier host work. Other inherited dirty
files retain their reviewed predecessor function identities unless explicitly
listed in the new delta. A Git diff against HEAD includes earlier work and is not
the TASK0075 baseline; the strict predecessor-derived identity delta is authoritative.

## Pinning and validation

Draft review files contain no approved current source fingerprints. Run
`py -3 reverse/scripts/audit_task0075_review.py --delta` to inspect exact changes.
After the final host changes and explicit scope review, root may run `--write`
to pin only the allowlisted identities and auxiliary files. Historical review and
scope files must not be refreshed. Source approval does not establish original
runtime equivalence or Verified status.

Root validation: new native tests4/4, existing native regressions10/10,
headless production-host/projection regressions7/7 and two real Vulkan particle
fixtures passed. The latter report469/467 (flat) and497/398 (Track11) peak
active/visible particles with no reported gaps in those runs. Current-velocity
MotionLive correction is separately evidenced by E-0075-D02; outside-view dynamic
render admission by E-0075-D03. Original probe timed out. Exact commands, tool
versions, remaining gaps and continuation are retained in TASK-0075 and
DRIVING_EFFECTS.md. These checks establish port regression coverage, not Verified.

