"""TASK0064 presentation-only explicit successor."""
import json
import audit_task0063_review as predecessor
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=predecessor.ROOT
REVIEW=ROOT/'reverse/task0064/source-review.json'
SCOPE=ROOT/'reverse/task0064/review-scope.json'
PREDECESSOR_REVIEW_SHA256='beb7486eab15a531e0ccea7dc9b9ad9e8f18188fa051c4d9312aaac62964da16'
PREDECESSOR_SCOPE_SHA256='1b0193c0b3893f3bbfd618cec4a0eee6fbd776e21f16011143f208194105ebba'

def historical_identities():
    assert predecessor.fingerprint(predecessor.REVIEW)==PREDECESSOR_REVIEW_SHA256
    assert predecessor.fingerprint(predecessor.SCOPE)==PREDECESSOR_SCOPE_SHA256
    result=transition.reviewed_identities(predecessor.historical_identities(),json.loads(predecessor.REVIEW.read_text()))
    auxiliary={row['path']:row['lf_sha256'] for row in json.loads(predecessor.REVIEW.read_text())['auxiliary_sources']}
    assert predecessor.check_current(current_identities=result,auxiliary_fingerprints=auxiliary)['passed']
    return result

def check_current(*,current_identities=None):
    if current_identities is None:
        from audit_task0065_review import check_current as latest
        return latest()
    before=historical_identities()
    review=json.loads(REVIEW.read_text());scope=json.loads(SCOPE.read_text())
    assert review['task']==scope['task']=='TASK-0064' and review['status']=='approved'
    assert review['target_binary_sha256']==identities.TARGET_SHA256
    assert review['predecessor_review_lf_sha256']==PREDECESSOR_REVIEW_SHA256
    assert review['baseline_inventory_sha256']==identities.inventory_digest(before)
    assert review['scope_lf_sha256']==predecessor.fingerprint(SCOPE)
    allowed={('src/platform/godot_bridge/ps2_render_host.cpp', 'Ps2RenderHost::viewer_ui_snapshot'), ('src/game_core/extensions/viewer_ui.cpp', 'ViewerUi::draw')}
    assert {(r['path'],r['name']) for r in scope['entries']}==allowed
    slots={(r['path'],r['name']):r for r in scope['entries']}
    for side in ('before','after'):
        assert len(review[side])==2
        for row in review[side]:
            slot=slots[(row['path'],row['name'])]
            assert row.get('stub',False)==False
            for key in ('kind','evidence_id','evidence_record','reason'):assert row[key]==slot[key]
            assert slot['kind']=='port_adapter' and slot['evidence_id']=='E0064U01' and slot['reason']
            assert slot['evidence_id'] in (ROOT/slot['evidence_record']).read_text()
    current=identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment=identities.check_identity_review(before,current,review)
    return dict(passed=amendment['passed'],task0064_amendment=amendment)

if __name__=='__main__':
    result=check_current();print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
