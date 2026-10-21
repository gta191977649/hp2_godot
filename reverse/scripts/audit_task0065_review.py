"""TASK0065 explicit port-only successor; frozen reviews are not refreshed."""
import json
import audit_task0064_review as predecessor
import audit_task0063_review as frozen
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=predecessor.ROOT
REVIEW=ROOT/'reverse/task0065/source-review.json'
SCOPE=ROOT/'reverse/task0065/review-scope.json'
PREDECESSOR_REVIEW_SHA256='5311b782baab28f8de4897fa0c6df609880c83548e5dd4f991432b8819c30df3'
PREDECESSOR_SCOPE_SHA256='08817efc72d7cb29ecc55eb7d9413c227997252ef9e64b9436cce69d1d913d38'
ALLOWED={('src/platform/godot_bridge/track_optimization.hpp', 'startup_track_optimizations'), ('src/platform/godot_bridge/track_mesh_batches_tests.cpp', 'test_track_mesh_batch_cache'), ('src/game_core/extensions/viewer_ui.cpp', 'ViewerUi::draw'), ('src/platform/godot_bridge/ps2_render_host.cpp', 'Ps2RenderHost::viewer_ui_snapshot'), ('src/platform/godot_bridge/track_sun_renderer.cpp', 'create_sun_target'), ('src/platform/godot_bridge/track_world_batches.hpp', 'eligible_track_world_batch'), ('src/platform/godot_bridge/ps2_track_renderer.cpp', 'Ps2TrackRenderer::submit_frame'), ('src/platform/godot_bridge/track_optimization.hpp', 'optimization_options_from_dictionary'), ('src/platform/godot_bridge/track_optimization.hpp', 'optimization_options_dictionary'), ('src/platform/godot_bridge/track_world_batches.hpp', 'contiguous_track_world_batches'), ('src/platform/godot_bridge/track_reflection_renderer.cpp', 'submit'), ('src/platform/godot_bridge/ps2_track_renderer.cpp', 'Ps2TrackRenderer::get_optimization_state'), ('src/platform/godot_bridge/track_mesh_batches.cpp', 'TrackMeshBatchCache::acquire')}
AUXILIARY={'src/tests/track_world_batches_tests.cpp', 'godot/scripts/debug/track_render_diagnostics_smoke.gd', 'src/tests/track_draw_inventory.cpp', 'godot/scripts/debug/track_render_diagnostics.gd', 'src/game_core/extensions/viewer_ui.hpp', 'src/platform/godot_bridge/track_optimization.hpp', 'godot/scripts/runtime/track_optimization_reflection_smoke.gd', 'src/platform/godot_bridge/track_command_batches.hpp', 'src/platform/godot_bridge/track_mesh_batches.cpp', 'godot/scripts/runtime/track_optimization_smoke.gd', 'godot/scripts/debug/asset_viewer.gd'}

def require(value,reason):
    if not value:raise ValueError(reason)

def historical_identities():
    require(frozen.fingerprint(predecessor.REVIEW)==PREDECESSOR_REVIEW_SHA256,'Frozen TASK0064 review changed')
    require(frozen.fingerprint(predecessor.SCOPE)==PREDECESSOR_SCOPE_SHA256,'Frozen TASK0064 scope changed')
    result=transition.reviewed_identities(predecessor.historical_identities(),json.loads(predecessor.REVIEW.read_text()))
    require(predecessor.check_current(current_identities=result)['passed'],'Frozen predecessor state failed')
    return result

def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    require(auxiliary_fingerprints is None or current_identities is not None,'Historical auxiliary snapshots require explicit identities')
    require(auxiliary_fingerprints is None or set(auxiliary_fingerprints)==AUXILIARY,'Historical auxiliary snapshot has wrong paths')
    if current_identities is None:
        from audit_task0066_review import check_current as latest
        return latest()
    before=historical_identities();review=json.loads(REVIEW.read_text());scope=json.loads(SCOPE.read_text())
    require(review.get('task')==scope.get('task')=='TASK-0065' and review.get('status')=='approved','Wrong approval/task')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_REVIEW_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    require(review.get('scope_lf_sha256')==frozen.fingerprint(SCOPE),'Changed scope')
    slots={(r['path'],r['name']):r for r in scope['entries']}
    require(set(slots)==ALLOWED and len(slots)==len(scope['entries']),'Wrong scope slots')
    for side in ('before','after'):
        for row in review[side]:
            slot=slots.get((row['path'],row['name']))
            require(slot is not None and not row.get('stub',False),'Unapproved function/stub')
            for key in ('kind','evidence_id','evidence_record','reason'):require(row.get(key)==slot.get(key),'Evidence mismatch')
            require(slot.get('kind')=='port_adapter' and slot.get('evidence_id')=='E0065H01' and slot.get('reason'),'Wrong owner')
            require(slot['evidence_id'] in (ROOT/slot['evidence_record']).read_text(),'Missing evidence')
    auxiliary={r['path']:r for r in scope['auxiliary_sources']}
    require(set(auxiliary)==AUXILIARY and len(auxiliary)==len(scope['auxiliary_sources']),'Wrong auxiliary scope')
    require(len(review['auxiliary_sources'])==len(AUXILIARY) and {r['path'] for r in review['auxiliary_sources']}==AUXILIARY,'Wrong auxiliary review')
    for row in review['auxiliary_sources']:
        slot=auxiliary[row['path']]
        for key in ('kind','evidence_id','evidence_record','reason'):require(row.get(key)==slot.get(key),'Auxiliary evidence mismatch')
        actual=frozen.fingerprint(ROOT/row['path']) if auxiliary_fingerprints is None else auxiliary_fingerprints.get(row['path'])
        require(actual==row.get('lf_sha256'),'Changed auxiliary source '+row['path'])
    current=identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment=identities.check_identity_review(before,current,review)
    pins={identities._identity_key(r):bool(r.get('stub')) for r in transition.reviewed_identities(before,review)}
    require(all(identities._identity_key(r) not in pins or pins[identities._identity_key(r)]==bool(r.get('stub')) for r in current),'Changed stub classification')
    return dict(passed=amendment['passed'],task0065_amendment=amendment)

if __name__=='__main__':
    result=check_current();print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
