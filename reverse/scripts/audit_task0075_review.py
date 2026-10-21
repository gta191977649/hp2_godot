"""TASK0075 strict wheel-effect/source successor; historical pins remain immutable."""
import argparse
from collections import Counter
from contextlib import contextmanager
import json
import sys
import audit_task0072_review as predecessor
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=predecessor.ROOT
REVIEW=ROOT/'reverse/task0075/source-review.json'
SCOPE=ROOT/'reverse/task0075/review-scope.json'
EVIDENCE='reverse/task0075/source-review.md'
PREDECESSOR_REVIEW_SHA256='d27b07fcb40a38d3d47720d1300f6dbfc17b321b7a95130de2bc7c30e9ce054d'
PREDECESSOR_SCOPE_SHA256='f4962e123d616fc6cd241c092abe53b18e75f13a3ad4d23ea4c2fa0aaf405443'
fingerprint=predecessor.fingerprint
require=predecessor.require
ALLOWED={
 ('src/game_core/indep/src/car.cpp','trigger_wheel_terrain_effects'),
 ('src/game_core/indep/src/car.cpp','Car::TriggerCarTerrainEffects'),
 ('src/game_core/port/driving/car_effects.cpp','original_text'),
 ('src/game_core/port/driving/car_effects.cpp','WheelParticleEffects::prepare'),
 ('src/game_core/port/driving/car_effects.cpp','WheelParticleEffects::trigger'),
 ('src/game_core/port/driving/native_backend.hpp','visual_effects'),
 ('src/game_core/port/track/assets.cpp','load_scene'),
 ('src/game_core/port/track/assets.cpp','append_particle_library'),
 ('src/game_core/port/track/particles.cpp','seed_'),
 ('src/game_core/port/track/particles.cpp','ParticleSimulation::clear_dynamic_emitters'),
 ('src/game_core/port/track/particles.cpp','ParticleSimulation::add_dynamic_emitter'),
 ('src/game_core/port/track/particles.cpp','ParticleSimulation::trigger_dynamic'),
 ('src/game_core/port/track/particles.cpp','ParticleSimulation::step'),
 ('src/game_core/port/track/particles.hpp','auto_emitter_order'),
 ('src/game_core/port/track/particles.hpp','set_car_velocity'),
 ('src/game_core/src/care.cpp','Car::TriggerEffect'),
 ('src/platform/godot_bridge/driving_particle_display.cpp','upload'),
 ('src/platform/godot_bridge/driving_particle_display.cpp','DrivingParticleDisplay::present'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::clear_particles'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::present_particles'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::particle_timestep'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::emit_wheel_particles'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::_bind_methods'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::load_config'),
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::action'),
 ('src/platform/godot_bridge/ps2_driving_runtime.hpp','particle_step'),
 ('src/platform/godot_bridge/ps2_driving_runtime.hpp','particle_velocity'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::_bind_methods'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::update_view'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::set_driving_effect_source'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp','TrackParticleRenderer::set_driving_source'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp','TrackParticleRenderer::clear'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp','TrackParticleRenderer::update'),
}
EVIDENCE_BY_SLOT={
 ('src/game_core/indep/src/car.cpp','trigger_wheel_terrain_effects'):'E-0075-T01',
 ('src/game_core/indep/src/car.cpp','Car::TriggerCarTerrainEffects'):'E-0075-T01',
 ('src/game_core/port/driving/car_effects.cpp','original_text'):'E-0075-C01',
 ('src/game_core/port/driving/car_effects.cpp','WheelParticleEffects::prepare'):'E-0075-C01',
 ('src/game_core/port/driving/car_effects.cpp','WheelParticleEffects::trigger'):'E-0075-C01',
 ('src/game_core/port/driving/native_backend.hpp','visual_effects'):'E-0075-R01',
 ('src/game_core/port/track/assets.cpp','load_scene'):'E-0075-R01',
 ('src/game_core/port/track/assets.cpp','append_particle_library'):'E-0075-R01',
 ('src/game_core/port/track/particles.cpp','seed_'):'E-0075-R01',
 ('src/game_core/port/track/particles.cpp','ParticleSimulation::clear_dynamic_emitters'):'E-0075-R01',
 ('src/game_core/port/track/particles.cpp','ParticleSimulation::add_dynamic_emitter'):'E-0075-R01',
 ('src/game_core/port/track/particles.cpp','ParticleSimulation::trigger_dynamic'):'E-0075-R01',
 ('src/game_core/port/track/particles.cpp','ParticleSimulation::step'):'E-0075-D02',
 ('src/game_core/port/track/particles.hpp','auto_emitter_order'):'E-0075-R01',
 ('src/game_core/port/track/particles.hpp','set_car_velocity'):'E-0075-D02',
 ('src/game_core/src/care.cpp','Car::TriggerEffect'):'E-0075-C01',
 ('src/platform/godot_bridge/driving_particle_display.cpp','upload'):'E-0075-R01',
 ('src/platform/godot_bridge/driving_particle_display.cpp','DrivingParticleDisplay::present'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::clear_particles'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::present_particles'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::particle_timestep'):'E-0075-P03',
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::emit_wheel_particles'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::_bind_methods'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::load_config'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_driving_runtime.cpp','Ps2DrivingRuntime::action'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_driving_runtime.hpp','particle_step'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_driving_runtime.hpp','particle_velocity'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::_bind_methods'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::update_view'):'E-0075-R01',
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::set_driving_effect_source'):'E-0075-R01',
 ('src/platform/godot_bridge/track_particle_renderer.cpp','TrackParticleRenderer::set_driving_source'):'E-0075-R01',
 ('src/platform/godot_bridge/track_particle_renderer.cpp','TrackParticleRenderer::clear'):'E-0075-R01',
 ('src/platform/godot_bridge/track_particle_renderer.cpp','TrackParticleRenderer::update'):'E-0075-R01',
}
AUXILIARY=predecessor.AUXILIARY|{
 'reverse/scripts/audit_task0072_review.py',
 'reverse/scripts/audit_task0075_review.py',
 'reverse/tests/test_task0075_source_review.py',
 'reverse/task0075/source-review.md',
 'reverse/task0075/terrain-producer.md',
 'reverse/task0075/dynamic-particles.md',
 'reverse/task0075/positions-scheduling.md',
 'src/game_core/indep/inc/Car.hpp',
 'src/game_core/port/driving/terrain_effects.hpp',
 'src/game_core/port/driving/car_effects.hpp',
 'src/game_core/port/driving/native_backend.hpp',
 'src/game_core/port/track/particles.hpp',
 'src/game_core/port/track/assets.hpp',
 'src/platform/godot_bridge/driving_particle_display.hpp',
 'src/platform/godot_bridge/ps2_driving_runtime.hpp',
 'src/tests/driving_terrain_effects_tests.cpp',
 'src/tests/driving_particle_tests.cpp',
 'godot/scripts/debug/driving_particles_smoke.gd',
 'godot/scripts/debug/track_smoke.gd',
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
    require(fingerprint(predecessor.REVIEW)==PREDECESSOR_REVIEW_SHA256,'Frozen TASK0072 review changed')
    require(fingerprint(predecessor.SCOPE)==PREDECESSOR_SCOPE_SHA256,'Frozen TASK0072 scope changed')
    review=json.loads(predecessor.REVIEW.read_text())
    result=transition.reviewed_identities(predecessor.historical_identities(),review)
    auxiliary={r['path']:r['lf_sha256'] for r in review['auxiliary_sources']}
    require(predecessor.check_current(current_identities=result,auxiliary_fingerprints=auxiliary)['passed'],'Historical predecessor failed')
    return result


def _slot(row):
    return row['path'],row['name']


def _annotation(slot=None):
    evidence=EVIDENCE_BY_SLOT.get(slot,'E-0075-R01')
    record={'E-0075-T01':'reverse/task0075/terrain-producer.md',
            'E-0075-C01':'reverse/task0075/terrain-producer.md',
            'E-0075-P03':'reverse/task0075/positions-scheduling.md',
            'E-0075-D02':'reverse/task0075/dynamic-particles.md',
            'E-0075-R01':'reverse/task0075/source-review.md'}[evidence]
    partial=slot==('src/game_core/indep/src/car.cpp','Car::TriggerCarTerrainEffects')
    recovered=slot in {('src/game_core/indep/src/car.cpp','trigger_wheel_terrain_effects'),('src/game_core/src/care.cpp','Car::TriggerEffect')}
    return dict(kind='partial_boundary' if partial else 'bounded_recovery' if recovered else 'port_adapter',
                evidence_id=evidence,evidence_record=record,
                reason='Full entry remains incomplete and explicitly throws after the recovered wheel loop; no whole-entry implementation claim.' if partial else
                'Explicit TASK0075 wheel-effect recovery or host storage/render/scheduling adaptation; original equivalence remains unverified.')


def _aux_annotation(path):
    result=_annotation()
    if path=='build.ps1':
        result['kind']='preserved_prior_state'
        result['reason']='Pre-existing working-tree build helper differed from TASK0072 at TASK0075 startup; preserved without modification, not a TASK0075 implementation.'
    elif path=='godot/scripts/debug/asset_viewer.gd':
        result['reason']='TASK0075 browsing projection and wheel display glue share this whole-file pin with pre-existing TASK0074 wireframe controls; prior wireframe is not recovered physics evidence.'
    return result


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
    scope=dict(task='TASK-0075',entries=[dict(path=path,name=name,**_annotation((path,name))) for path,name in sorted(slots)],
               auxiliary_sources=[dict(path=path,**_aux_annotation(path)) for path in sorted(AUXILIARY)])
    SCOPE.write_text(json.dumps(scope,indent=2)+'\n',encoding='utf-8')
    review=dict(task='TASK-0075',status='approved',target_binary_sha256=identities.TARGET_SHA256,
                predecessor_review_lf_sha256=PREDECESSOR_REVIEW_SHA256,
                baseline_inventory_sha256=identities.inventory_digest(before),scope_lf_sha256=fingerprint(SCOPE),
                before=changed[0],after=changed[1],
                auxiliary_sources=[dict(path=path,lf_sha256=fingerprint(ROOT/path),**_aux_annotation(path)) for path in sorted(AUXILIARY)])
    REVIEW.write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
    return check_current(current_identities=current)


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0076_review import check_current as latest
        return latest()
    require(auxiliary_fingerprints is None or current_identities is not None,'Historical auxiliary state requires explicit identities')
    require(auxiliary_fingerprints is None or set(auxiliary_fingerprints)==AUXILIARY,'Wrong auxiliary snapshot paths')
    before=historical_identities();review=json.loads(REVIEW.read_text());scope=json.loads(SCOPE.read_text())
    require(review.get('task')==scope.get('task')=='TASK-0075' and review.get('status')=='approved','Wrong task/approval')
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
    require('E-0075-R01' in (ROOT/EVIDENCE).read_text(),'Missing host evidence')
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
    return dict(passed=amendment['passed'],task0075_amendment=amendment)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write',action='store_true',help='Explicitly pin the settled allowlisted host changes')
    parser.add_argument('--delta',action='store_true',help='Inspect the unapproved current delta without writing pins')
    args=parser.parse_args()
    require(not (args.write and args.delta),'Choose either --write or --delta')
    result=inspect_delta() if args.delta else write_review() if args.write else check_current()
    print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
