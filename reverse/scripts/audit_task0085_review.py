"""Strict TASK0085 GPU particles successor; frozen predecessor JSON is immutable."""
from collections import Counter
import json
import audit_task0084_review as prior

ROOT = prior.ROOT
REVIEW = ROOT / 'reverse/task0085/source-review.json'
PREDECESSOR_SHA256 = '4c9f10509d1ddadfbd329f85e1cbbca398cfc2202af40a8682e52daf1d2a6b96'
REPLACED = {
 ('src/game_core/port/track/particles.cpp', 'ParticleSimulation::clear_dynamic_emitters'),
 ('src/game_core/port/track/particles.cpp', 'ParticleSimulation::step'),
 ('src/game_core/port/track/particles.cpp', 'ParticleSimulation::trigger_dynamic'),
 ('src/platform/godot_bridge/driving_particle_display.cpp', 'DrivingParticleDisplay::present'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp', 'Ps2TrackRenderer::update_view'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp', 'TrackParticleRenderer::clear'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp', 'TrackParticleRenderer::set_alpha_ranks'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp', 'TrackParticleRenderer::set_material_callbacks'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp', 'TrackParticleRenderer::stats'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp', 'TrackParticleRenderer::update'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp', 'impl_'),
}
ADDED = {
 ('src/game_core/port/track/particles.cpp', 'ParticleSimulation::advance_gpu'),
 ('src/game_core/port/track/particles.cpp', 'ParticleSimulation::capture_spawn'),
 ('src/game_core/port/track/particles.cpp', 'ParticleSimulation::set_gpu_emission'),
 ('src/game_core/port/track/particles.cpp', 'ParticleSimulation::take_gpu_commands'),
 ('src/game_core/port/track/particles.cpp', 'ParticleSimulation::trigger_gpu'),
 ('src/game_core/port/track/particles.hpp', 'gpu_commands'),
 ('src/game_core/port/track/particles.hpp', 'gpu_emission'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'GpuParticleRenderer::abort_frame'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'GpuParticleRenderer::clear'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'GpuParticleRenderer::commit_frame'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'GpuParticleRenderer::set_alpha_ranks'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'GpuParticleRenderer::set_material_callbacks'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'GpuParticleRenderer::stats'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'GpuParticleRenderer::update'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'alpha_peak'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'draw_source'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'impl_'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'matrix'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'mip'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'remove'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', 'shader_material'),
 ('src/platform/godot_bridge/gpu_particle_renderer.cpp', '~GpuParticleRenderer'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp', 'TrackParticleRenderer::abort_frame'),
 ('src/platform/godot_bridge/track_particle_renderer.cpp', 'TrackParticleRenderer::commit_frame'),
}
STUB_REPLACEMENTS = set()
CHANGED_AUXILIARY = {
 'CMakeLists.txt',
 'godot/project.godot',
 'godot/scripts/debug/driving_particles_smoke.gd',
 'godot/scripts/runtime/track_performance.gd',
 'godot/scripts/runtime/track_performance_viewer.gd',
 'reverse/scripts/audit_task0084_review.py',
 'reverse/tests/test_task0084_source_review.py',
 'src/game_core/port/track/particles.hpp',
 'src/platform/godot_bridge/ps2_track_renderer.cpp',
 'src/platform/godot_bridge/track_particle_renderer.cpp',
 'src/platform/godot_bridge/track_particle_renderer.hpp',
 'tools/run_track_costs.ps1',
}
ADDED_AUXILIARY = {
 'godot/scripts/debug/gpu_particle_lifecycle_smoke.gd',
 'godot/scripts/debug/gpu_particle_state_smoke.gd',
 'godot/scripts/debug/gpu_particle_visual_smoke.gd',
 'reverse/scripts/audit_task0085_review.py',
 'reverse/task0085/source-review.md',
 'reverse/tests/test_task0085_source_review.py',
 'src/platform/godot_bridge/gpu_particle_renderer.cpp',
 'src/platform/godot_bridge/gpu_particle_renderer.hpp',
 'src/tests/gpu_particle_emission_tests.cpp',
 'tools/particle_backend_benchmark.py',
}
identities = prior.identities
transition = prior.transition
fingerprint = prior.fingerprint
require = prior.require


def historical_state():
    require(fingerprint(prior.REVIEW)==PREDECESSOR_SHA256,'Frozen TASK0084 review changed')
    old,frozen = prior.historical_state()
    review = json.loads(prior.REVIEW.read_text())
    before = transition.reviewed_identities(old,review)
    frozen.update({path:row['after'] for path,row in review['replacements'].items()})
    frozen.update(review['additions'])
    require(prior.check_current(current_identities=before,auxiliary_fingerprints=frozen)['passed'],
            'Historical TASK0084 snapshot failed')
    return before,frozen


def validate(before,current,frozen,review,auxiliary):
    require(review.get('task')=='TASK-0085' and review.get('evidence')=='E-0085-S01','Wrong task/evidence')
    require(review.get('target_binary_sha256')==identities.TARGET_SHA256,'Wrong Debug target')
    require(review.get('predecessor_review_lf_sha256')==PREDECESSOR_SHA256,'Wrong predecessor')
    require(review.get('baseline_inventory_sha256')==identities.inventory_digest(before),'Wrong baseline')
    for side,scope in [('before',REPLACED),('after',REPLACED|ADDED)]:
        rows = review[side]
        require(len(rows)==len(scope) and {(r['path'],r['name']) for r in rows}==scope,'Wrong function scope')
        for row in rows:
            require(row.get('evidence_id')=='E-0085-S01' and bool(row.get('reason')),'Missing function evidence')
            expected = side=='before' and (row['path'],row['name']) in STUB_REPLACEMENTS
            require(bool(row.get('stub'))==expected,'Unexpected stub transition')
    result = identities.check_identity_review(before,current,review)
    require(result['passed'],'Unreviewed function identity change')
    successor = transition.reviewed_identities(before,review)
    pins = {identities._identity_key(r):bool(r.get('stub')) for r in successor}
    require(all(pins.get(identities._identity_key(r))==bool(r.get('stub')) for r in current),'Changed live stub classification')
    replacements,additions = review['replacements'],review['additions']
    require(set(replacements)==CHANGED_AUXILIARY and set(additions)==ADDED_AUXILIARY,'Wrong auxiliary scope')
    require(not ADDED_AUXILIARY & frozen.keys(),'Added auxiliary already inherited')
    expected = dict(frozen)
    for path,row in replacements.items():
        require(row['before']==frozen[path],'Wrong auxiliary baseline '+path)
        expected[path] = row['after']
    expected.update(additions)
    require(auxiliary==expected,'Unreviewed auxiliary mutation or missing path')
    return dict(passed=True,task='TASK-0085',replaced_functions=len(REPLACED),added_functions=len(ADDED),
                modified_auxiliary=len(replacements),added_auxiliary=len(additions))


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0086_review import check_current as latest
        return latest()
    require(current_identities is not None,'Historical auxiliary snapshot requires explicit identities')
    before,frozen = historical_state()
    require(REVIEW.is_file(),'TASK0085 source is not yet settled/reviewed')
    review = json.loads(REVIEW.read_text())
    current = current_identities
    auxiliary = ({p:fingerprint(ROOT/p) for p in set(frozen)|ADDED_AUXILIARY}
                 if auxiliary_fingerprints is None else auxiliary_fingerprints)
    require('E-0085-S01' in (ROOT/'reverse/task0085/source-review.md').read_text(),'Missing review evidence')
    return validate(before,current,frozen,review,auxiliary)


def inspect_delta():
    before,frozen = historical_state()
    current = identities.source_identities(identities.historical.collect_current())
    old,new = (Counter(identities._identity_key(r) for r in rows) for rows in (before,current))
    return dict(before=[r for r in before if (old-new)[identities._identity_key(r)]],
                after=[r for r in current if (new-old)[identities._identity_key(r)]],
                auxiliary=sorted(p for p,h in frozen.items() if fingerprint(ROOT/p)!=h))


if __name__=='__main__':
    print(json.dumps(check_current(),indent=2))
