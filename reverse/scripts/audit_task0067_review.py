"""TASK0067 live primitive overlay; historical review bytes remain immutable."""
import json
import audit_task0066_review as predecessor
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=predecessor.ROOT
REVIEW=ROOT/'reverse/task0067/source-review.json'
SCOPE=ROOT/'reverse/task0067/review-scope.json'
PREDECESSOR_REVIEW_SHA256='b98f2f74bbc734ecb2a30eb5270df109d8311befcf153d8babd8f14f2cb08004'
PREDECESSOR_SCOPE_SHA256='afc2d00815ce117516e87778d28d8b2c433757dca3030418d2fc223d05105b69'
ALLOWED={('src/game_core/extensions/viewer_ui.cpp', 'ViewerUi::draw'), ('src/platform/godot_bridge/ps2_render_host.cpp', 'Ps2RenderHost::viewer_ui_snapshot')}
AUXILIARY={'src/game_core/extensions/viewer_ui.hpp', 'reverse/scripts/audit_task0066_review.py', 'godot/scripts/runtime/track_optimization_smoke.gd', 'reverse/scripts/audit_task0065_review.py', 'godot/scripts/debug/asset_viewer.gd', 'src/tests/track_draw_inventory.cpp', 'godot/scripts/debug/track_render_diagnostics_smoke.gd', 'godot/scripts/runtime/track_optimization_reflection_smoke.gd', 'src/platform/godot_bridge/track_command_batches.hpp', 'src/platform/godot_bridge/track_mesh_batches.cpp', 'godot/scripts/debug/track_render_diagnostics.gd', 'reverse/tests/test_task0065_source_review.py', 'src/platform/godot_bridge/track_optimization.hpp', 'src/tests/track_world_batches_tests.cpp'}
fingerprint=predecessor.fingerprint
require=predecessor.require

def historical_identities():
    require(fingerprint(predecessor.REVIEW)==PREDECESSOR_REVIEW_SHA256,'Frozen TASK0066 review changed')
    require(fingerprint(predecessor.SCOPE)==PREDECESSOR_SCOPE_SHA256,'Frozen TASK0066 scope changed')
    review=json.loads(predecessor.REVIEW.read_text())
    result=transition.reviewed_identities(predecessor.historical_identities(),review)
    auxiliary={r['path']:r['lf_sha256'] for r in review['auxiliary_sources']}
    require(predecessor.check_current(current_identities=result,auxiliary_fingerprints=auxiliary)['passed'],'Historical predecessor failed')
    return result

def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0069_review import check_current as latest
        return latest()
    require(auxiliary_fingerprints is None or current_identities is not None,'Historical auxiliary state requires explicit identities')
    require(auxiliary_fingerprints is None or set(auxiliary_fingerprints)==AUXILIARY,'Wrong auxiliary snapshot paths')
    before=historical_identities();review=json.loads(REVIEW.read_text());scope=json.loads(SCOPE.read_text())
    require(review.get('task')==scope.get('task')=='TASK-0067' and review.get('status')=='approved','Wrong task/approval')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_REVIEW_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    require(review.get('scope_lf_sha256')==fingerprint(SCOPE),'Changed scope')
    slots={(r['path'],r['name']):r for r in scope['entries']}
    require(set(slots)==ALLOWED and len(slots)==len(scope['entries']),'Wrong scope slots')
    for side in ('before','after'):
        require(len(review[side])==2,'Wrong replacement count')
        for row in review[side]:
            slot=slots.get((row['path'],row['name']))
            require(slot is not None and not row.get('stub',False),'Unapproved function/stub')
            for key in ('kind','evidence_id','evidence_record','reason'):require(row.get(key)==slot.get(key),'Evidence mismatch')
            require(slot.get('kind')=='port_adapter' and slot.get('evidence_id')=='E0067U01' and slot.get('reason'),'Wrong owner')
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
    return dict(passed=amendment['passed'],task0067_amendment=amendment)

if __name__=='__main__':
    result=check_current();print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
