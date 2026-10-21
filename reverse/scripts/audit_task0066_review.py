"""TASK0066 global port policy; historical review bytes remain immutable."""
import json
import audit_task0065_review as predecessor
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=predecessor.ROOT
REVIEW=ROOT/'reverse/task0066/source-review.json'
SCOPE=ROOT/'reverse/task0066/review-scope.json'
PREDECESSOR_REVIEW_SHA256='af0cc7f2e391471187383e72816cb910c0ca54e0c70addf8ea8a1da3b11170c2'
PREDECESSOR_SCOPE_SHA256='84545666ed2978115f217e970e2495896ae72d4472077d68d0f463331c08ff50'
ALLOWED={('src/platform/godot_bridge/ps2_track_renderer.cpp', 'Ps2TrackRenderer::get_optimization_state'), ('src/platform/godot_bridge/ps2_track_renderer.cpp', 'Ps2TrackRenderer::submit_frame'), ('src/platform/godot_bridge/track_optimization.hpp', 'optimization_options_from_dictionary')}
AUXILIARY={'reverse/tests/test_task0065_source_review.py', 'godot/scripts/debug/track_render_diagnostics_smoke.gd', 'godot/scripts/debug/asset_viewer.gd', 'src/platform/godot_bridge/track_command_batches.hpp', 'src/platform/godot_bridge/track_mesh_batches.cpp', 'reverse/scripts/audit_task0065_review.py', 'godot/scripts/runtime/track_optimization_smoke.gd', 'src/game_core/extensions/viewer_ui.hpp', 'godot/scripts/debug/track_render_diagnostics.gd', 'src/platform/godot_bridge/track_optimization.hpp', 'src/tests/track_draw_inventory.cpp', 'src/tests/track_world_batches_tests.cpp', 'godot/scripts/runtime/track_optimization_reflection_smoke.gd'}
fingerprint=predecessor.frozen.fingerprint
require=predecessor.require

def historical_identities():
    require(fingerprint(predecessor.REVIEW)==PREDECESSOR_REVIEW_SHA256,'Frozen TASK0065 review changed')
    require(fingerprint(predecessor.SCOPE)==PREDECESSOR_SCOPE_SHA256,'Frozen TASK0065 scope changed')
    review=json.loads(predecessor.REVIEW.read_text())
    result=transition.reviewed_identities(predecessor.historical_identities(),review)
    auxiliary={r['path']:r['lf_sha256'] for r in review['auxiliary_sources']}
    require(predecessor.check_current(current_identities=result,auxiliary_fingerprints=auxiliary)['passed'],'Historical predecessor failed')
    return result

def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    require(auxiliary_fingerprints is None or current_identities is not None,'Historical auxiliary state requires explicit identities')
    require(auxiliary_fingerprints is None or set(auxiliary_fingerprints)==AUXILIARY,'Wrong auxiliary snapshot paths')
    if current_identities is None:
        from audit_task0067_review import check_current as latest
        return latest()
    before=historical_identities();review=json.loads(REVIEW.read_text());scope=json.loads(SCOPE.read_text())
    require(review.get('task')==scope.get('task')=='TASK-0066' and review.get('status')=='approved','Wrong task/approval')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_REVIEW_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    require(review.get('scope_lf_sha256')==fingerprint(SCOPE),'Changed scope')
    slots={(r['path'],r['name']):r for r in scope['entries']}
    require(set(slots)==ALLOWED and len(slots)==len(scope['entries']),'Wrong scope slots')
    for side in ('before','after'):
        require(len(review[side])==3,'Wrong replacement count')
        for row in review[side]:
            slot=slots.get((row['path'],row['name']))
            require(slot is not None and not row.get('stub',False),'Unapproved function/stub')
            for key in ('kind','evidence_id','evidence_record','reason'):require(row.get(key)==slot.get(key),'Evidence mismatch')
            require(slot.get('kind')=='port_adapter' and slot.get('evidence_id')=='E0066H01' and slot.get('reason'),'Wrong owner')
            require(slot['evidence_id'] in (ROOT/slot['evidence_record']).read_text(),'Missing evidence')
    auxiliary={r['path']:r for r in scope['auxiliary_sources']}
    require(set(auxiliary)==AUXILIARY and len(auxiliary)==len(scope['auxiliary_sources']),'Wrong auxiliary scope')
    require(len(review['auxiliary_sources'])==len(AUXILIARY) and {r['path'] for r in review['auxiliary_sources']}==AUXILIARY,'Wrong auxiliary review')
    for row in review['auxiliary_sources']:
        for key in ('kind','evidence_id','evidence_record','reason'):require(row.get(key)==auxiliary[row['path']].get(key),'Auxiliary evidence mismatch')
        actual=fingerprint(ROOT/row['path']) if auxiliary_fingerprints is None else auxiliary_fingerprints.get(row['path'])
        require(actual==row.get('lf_sha256'),'Changed auxiliary source '+row['path'])
    current=identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment=identities.check_identity_review(before,current,review)
    pins={identities._identity_key(r):bool(r.get('stub')) for r in transition.reviewed_identities(before,review)}
    require(all(identities._identity_key(r) not in pins or pins[identities._identity_key(r)]==bool(r.get('stub')) for r in current),'Changed stub classification')
    return dict(passed=amendment['passed'],task0066_amendment=amendment)

if __name__=='__main__':
    result=check_current();print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
