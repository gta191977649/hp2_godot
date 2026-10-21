# E0066H01 - Global matrix batching policy

The user explicitly rejected per-track optimization allowlists and requested enabling the optimization on all tracks. Remove world_batches_validated_only and the Track11 numeric predicate; the existing optimized profile enables the same state-based planner on every track. Remove the obsolete track11-only diagnostic field. Retain generic eligibility and car/capture/owner/LOD state boundaries; do not enable unrelated rejected opaque/MultiMesh experiments.

No native selection, LOD, effects, RNG, physics, shader arithmetic, matrix cache transport or command order changes. Track24 default now exercises real batching:764->266 and799->231 total draws at the first two marker views. The known second-view colour mismatch remains (native depth and native packets matched); this rollout is not an all-track pixel-equivalence claim. The global policy change is explicitly user requested rather than hidden behind another track/material-ID list. No prolonged FPS test.

Frozen TASK0065 review/scope and its full auxiliary hash inventory remain unchanged. Its historical checker now accepts an explicit complete auxiliary snapshot with explicit function identities, allowing a successor to verify historical authorized bytes instead of requiring the current workspace to remain at old header bytes. Live checks delegate to this successor, which pins its own changed auxiliary sources. Historical framework tests pass the frozen hash map; existing negative body/stub tests remain fail-closed.

The successor carries every inherited TASK0065 auxiliary pin forward in its live scope, plus the changed historical checker/tests; explicit historical snapshots therefore cannot hide new changes in unchanged inherited auxiliary files.
