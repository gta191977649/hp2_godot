# TASK-0066 - Global matrix batching policy

- Status: completed
- User explicitly requests global optimizations with no hardcoded per-track allowlist. Remove the Track11-only policy and diagnostics; optimized profile enables the same state-based planner for every track.
- Preserve generic lighting/alpha/order/capture-consumer/owner/LOD correctness boundaries and independent rollback option. No long FPS benchmark.
- Investigate the known Track24 pixel mismatch with a bounded capture; report remaining differences honestly. Do not add a different track-ID exception.
- Validate native boundaries, normal Debug build, actual default counters on multiple tracks, and source-preservation successor. Historical source reviews remain frozen; predecessor auxiliary state must support explicit frozen-snapshot validation.

- Delivered: removed track allowlist/config/diagnostic; every track uses the same default matrix batching policy. Kept generic state fallbacks and rollback; no new ID exceptions.
- Normal Debug build passes;14targeted native/UI/host/source checks pass. Three Track11 marker pairs are exact. Track24 default now reduces764->266 and799->231draws; second-view colour discrepancy is still reported as a failing image check (depth/native packets match). No all-track image/FPS claim.
- Immutable TASK0065 review/scope unchanged. TASK0066 pins three exact before/after function replacements, inherited auxiliary files and historical-checker support.
- Future continuation: resolve the colour discrepancy using common shader/state rules without reintroducing a track list. See docs/PERFORMANCE_TASK0066.md.
