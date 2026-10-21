# Global matrix batching policy (TASK-0066)

The user explicitly requested global optimization and rejected a hardcoded
Track11 allowlist. The optimized profile now enables matrix batching for every
track. Removed world_batches_validated_only, the number==11 condition and the
obsolete world_batches_track11_only diagnostic. No per-track replacement list.

All tracks use the same eligibility: contiguous unlit Copy state, matching
material/texture/light context, section/LOD boundaries, original alpha/order
barriers and bounded caches. Car visual consumers, capture views, nonidentity
owners and static LOD inspection retain the same generic baseline fallback.
Unrelated opaque/MultiMesh experiments remain off. Global rollback remains
`--track-opt-world_batches=false`; the baseline profile remains available.

## Bounded validation

Normal `./build.ps1` updates/validates the root DLL used by `./build.ps1 --run`.
14targeted native/UI/host/source checks pass. TASK0066 explicitly replaces three
port function bodies and preserves frozen source reviews and inherited auxiliary
pins. No recovered game/physics/native selection/shader arithmetic was changed.

| Actual default view | Previous draws | Global default draws | Image result |
|---|---:|---:|---|
| Track11 marker0 |219|157|Exact final/colour/depth|
| Track11 marker+50 |196|138|Exact final/colour/depth|
| Track11 marker-25 |237|169|Exact final/colour/depth|
| Track24 marker0 |764|266|Exact final/colour/depth|
| Track24 marker+50 |799|231|Known colour mismatch; depth/native packets match|

The Track24 second-view image test deliberately still fails: enabling globally
is not a claim that its previously recorded image discrepancy is repaired.
The retained difference is in colour (up to4byte levels in the examined pair),
not encoded depth; its shader/driver root cause is unresolved. No increased
image tolerance or hidden per-track disable was introduced. Prior Track11 dense
results and30-track CPU census remain in PERFORMANCE_TASK0065.md; neither is a
complete30-track GPU image/FPS acceptance run. Logs are local/task0066/.

Future rendering optimizations must use global state-based rules, never a
hardcoded track-ID allowlist. Track IDs remain legitimate for reproducible test
fixtures and reports. No prolonged FPS benchmark or engine/backend change.
