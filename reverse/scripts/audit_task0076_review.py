"""TASK0076 strict render-retention/source successor; historical pins remain immutable."""
import argparse
from collections import Counter
from contextlib import contextmanager
import json
import sys
import audit_task0075_review as predecessor
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=predecessor.ROOT
REVIEW=ROOT/'reverse/task0076/source-review.json'
SCOPE=ROOT/'reverse/task0076/review-scope.json'
EVIDENCE='reverse/task0076/source-review.md'
PREDECESSOR_REVIEW_SHA256='5d7d4f4afb8bfc620ef6a0a9b06a70d7bfa9419489c7f45727fb263666aaf76c'
PREDECESSOR_SCOPE_SHA256='55f6152f380ddaa9558b318bae5a580158bc4a461ed08888f7c7d86f1ebdf9ae'
fingerprint=predecessor.fingerprint
require=predecessor.require
ALLOWED={
 ('src/game_core/port/track/settings.hpp','RenderSettings'),
 ('src/game_core/port/track/settings.hpp','restore'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::release_display'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::update_view'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::complete_visual_readback'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::get_state'),
}
EVIDENCE_BY_SLOT={slot:'E-0076-R01' for slot in ALLOWED}
AUXILIARY=predecessor.AUXILIARY|{
 'reverse/scripts/audit_task0075_review.py',
 'reverse/scripts/audit_task0076_review.py',
 'reverse/task0076/source-review.md',
 'godot/scripts/runtime/track_retention_smoke.gd',
 'godot/scripts/runtime/track_controls_smoke.gd',
 'godot/scripts/runtime/track_optimization_reflection_smoke.gd',
 'src/tests/track_effects_tests.cpp',
 'src/game_core/port/track/settings.hpp',
}

@contextmanager
def _memoized_history_pass():
    """Reuse completed historical snapshots only within this validation pass.

    Historical functions recursively validate their predecessor and then ask the
    same predecessor to validate the resulting explicit snapshot. Without scoped
    reuse that repeats the entire older tree exponentially. Every original
    function, frozen fingerprint and explicit historical check still executes;
    no result survives the pass, and exceptions are never cached. This CLI and
    its review tests run these module bindings on one thread.
    """
    replaced=[]
    def memoize(original):
        ready=False
        value=None
        def once():
            nonlocal ready,value
            if not ready:
                value=original()
                ready=True
            return value
        return once
    try:
        for name,module in tuple(sys.modules.items()):
            if not name.startswith('audit_task') or module is sys.modules.get(__name__):
                continue
            original=getattr(module,'historical_identities',None)
            if callable(original):
                replaced.append((module,original))
                module.historical_identities=memoize(original)
        yield
    finally:
        for module,original in reversed(replaced):
            module.historical_identities=original


def historical_identities():
    with _memoized_history_pass():
        return _historical_identities_uncached()


def _historical_identities_uncached():
    require(fingerprint(predecessor.REVIEW)==PREDECESSOR_REVIEW_SHA256,'Frozen TASK0075 review changed')
    require(fingerprint(predecessor.SCOPE)==PREDECESSOR_SCOPE_SHA256,'Frozen TASK0075 scope changed')
    review=json.loads(predecessor.REVIEW.read_text())
    result=transition.reviewed_identities(predecessor.historical_identities(),review)
    auxiliary={r['path']:r['lf_sha256'] for r in review['auxiliary_sources']}
    require(predecessor.check_current(current_identities=result,auxiliary_fingerprints=auxiliary)['passed'],'Historical predecessor failed')
    return result


def _slot(row):
    return row['path'],row['name']


def _annotation(slot=None):
    return dict(kind='port_adapter',evidence_id='E-0076-R01',evidence_record=EVIDENCE,
                reason='User-authorized default-off dynamic reflection capture and non-destructive host render-gap lifetime/report/retry policy; recovered algorithms unchanged.')


def _aux_annotation(path):
    return _annotation()


def inspect_delta():
    """Report current changes without writing or approving any fingerprints."""
    before=historical_identities()
    current=identities.source_identities(identities.historical.collect_current())
    sets=[Counter(identities._identity_key(row) for row in rows) for rows in (before,current)]
    changed=[]
    for index,rows in enumerate((before,current)):
        delta=sets[index]-sets[1-index]
        changed.append([row for row in rows if delta[identities._identity_key(row)]])
    slots={_slot(row) for rows in changed for row in rows}
    return dict(passed=slots==ALLOWED and not any(row.get('stub') for rows in changed for row in rows),
                unapproved=True,before=changed[0],after=changed[1],
                unexpected_slots=sorted(slots-ALLOWED),missing_slots=sorted(ALLOWED-slots))


def write_review():
    before=historical_identities()
    current=identities.source_identities(identities.historical.collect_current())
    sets=[Counter(identities._identity_key(r) for r in rows) for rows in (before,current)]
    changed=[]
    for index,rows in enumerate((before,current)):
        delta=sets[index]-sets[1-index]
        changed.append([dict(row,**_annotation(_slot(row))) for row in rows if delta[identities._identity_key(row)]])
    slots={_slot(row) for rows in changed for row in rows}
    require(slots==ALLOWED,'Unreviewed identity changes: '+str({'unexpected':sorted(slots-ALLOWED),'missing':sorted(ALLOWED-slots)}))
    require(all(not r.get('stub') for rows in changed for r in rows),'Stub change is outside host scope')
    scope=dict(task='TASK-0076',entries=[dict(path=path,name=name,**_annotation((path,name))) for path,name in sorted(slots)],
               auxiliary_sources=[dict(path=path,**_aux_annotation(path)) for path in sorted(AUXILIARY)])
    SCOPE.write_text(json.dumps(scope,indent=2)+'\n',encoding='utf-8')
    review=dict(task='TASK-0076',status='approved',target_binary_sha256=identities.TARGET_SHA256,
                predecessor_review_lf_sha256=PREDECESSOR_REVIEW_SHA256,
                baseline_inventory_sha256=identities.inventory_digest(before),scope_lf_sha256=fingerprint(SCOPE),
                before=changed[0],after=changed[1],
                auxiliary_sources=[dict(path=path,lf_sha256=fingerprint(ROOT/path),**_aux_annotation(path)) for path in sorted(AUXILIARY)])
    REVIEW.write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
    return check_current(current_identities=current)


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0077_review import check_current as latest
        return latest()
    require(auxiliary_fingerprints is None or current_identities is not None,'Historical auxiliary state requires explicit identities')
    require(auxiliary_fingerprints is None or set(auxiliary_fingerprints)==AUXILIARY,'Wrong auxiliary snapshot paths')
    before=historical_identities();review=json.loads(REVIEW.read_text());scope=json.loads(SCOPE.read_text())
    require(review.get('task')==scope.get('task')=='TASK-0076' and review.get('status')=='approved','Wrong task/approval')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_REVIEW_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    require(review.get('scope_lf_sha256')==fingerprint(SCOPE),'Changed scope')
    slots={_slot(r):r for r in scope['entries']}
    require(set(slots)==ALLOWED and len(slots)==len(scope['entries']),'Wrong scope slots')
    require({_slot(r) for side in ('before','after') for r in review[side]}==set(slots),'Scope/review slots differ')
    for side in ('before','after'):
        for row in review[side]:
            require(not row.get('stub',False),'Stub replacement forbidden')
            require(all(row.get(k)==v and slots[_slot(row)].get(k)==v for k,v in _annotation(_slot(row)).items()),'Function evidence mismatch')
    require('E-0076-R01' in (ROOT/EVIDENCE).read_text(),'Missing host evidence')
    auxiliary={r['path']:r for r in scope['auxiliary_sources']}
    require(set(auxiliary)==AUXILIARY and len(auxiliary)==len(scope['auxiliary_sources']),'Wrong auxiliary scope')
    require(len(review['auxiliary_sources'])==len(AUXILIARY) and {r['path'] for r in review['auxiliary_sources']}==AUXILIARY,'Wrong auxiliary review')
    for row in review['auxiliary_sources']:
        require(all(row.get(k)==v and auxiliary[row['path']].get(k)==v for k,v in _aux_annotation(row['path']).items()),'Auxiliary evidence mismatch')
        actual=fingerprint(ROOT/row['path']) if auxiliary_fingerprints is None else auxiliary_fingerprints.get(row['path'])
        require(actual==row.get('lf_sha256'),'Changed auxiliary source '+row['path'])
    current=identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment=identities.check_identity_review(before,current,review)
    pins={identities._identity_key(r):bool(r.get('stub')) for r in transition.reviewed_identities(before,review)}
    require(all(identities._identity_key(r) not in pins or pins[identities._identity_key(r)]==bool(r.get('stub')) for r in current),'Changed stub classification')
    return dict(passed=amendment['passed'],task0076_amendment=amendment)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write',action='store_true',help='Explicitly pin the settled allowlisted host changes')
    parser.add_argument('--delta',action='store_true',help='Inspect the unapproved current delta without writing pins')
    args=parser.parse_args()
    require(not (args.write and args.delta),'Choose either --write or --delta')
    result=inspect_delta() if args.delta else write_review() if args.write else check_current()
    print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)


