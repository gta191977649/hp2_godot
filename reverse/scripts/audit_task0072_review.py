"""TASK0072 strict host-only source successor. Frozen prior reviews are immutable."""
import argparse
from collections import Counter
import json
import audit_task0071_review as predecessor
import audit_task0044_migration as identities
import audit_task0048_review as transition
ROOT=predecessor.ROOT
REVIEW=ROOT/'reverse/task0072/source-review.json'
SCOPE=ROOT/'reverse/task0072/review-scope.json'
EVIDENCE='reverse/task0072/host-rendering.md'
PREDECESSOR_REVIEW_SHA256='9201d3e01f7dfce22be05eef3f024f355103aa24be3f1d22e44e8d7cee49f2d0'
PREDECESSOR_SCOPE_SHA256='5b4dd99c25bb1608cbaad06e9890a248459a57deac7860522cfd8cccc910d77d'
fingerprint=predecessor.fingerprint
require=predecessor.require
ALLOWED={
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::get_state'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::get_render_controls'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::get_memory_state'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::acquire_source_mesh'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::get_optimization_state'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::submit_frame'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::update_view'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::release_display'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','Ps2TrackRenderer::trim_draw_cache'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','impl_'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::acquire'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::clear'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::uploaded_payload_bytes'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::release'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::stats'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::update'),
 ('src/platform/godot_bridge/track_postprocess.cpp','impl_'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::Impl::depth_material'),
 ('src/platform/godot_bridge/track_postprocess.cpp','TrackPostprocess::mark_material_dirty'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','impl_'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::layout_lods'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::pump_load'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::rebuild_display'),
 ('src/platform/godot_bridge/ps2_vehicle_renderer.cpp','Ps2VehicleRenderer::update_view'),
 ('src/platform/godot_bridge/parameter_staging_registry.hpp','create'),
 ('src/platform/godot_bridge/parameter_staging_registry.hpp','invalidate'),
 ('src/platform/godot_bridge/parameter_staging_registry.hpp','lookup'),
 ('src/platform/godot_bridge/parameter_staging_registry.hpp','publish'),
 ('src/platform/godot_bridge/parameter_staging_registry.hpp','stats'),
 ('src/platform/godot_bridge/vehicle_depth_staging.hpp','create'),
 ('src/platform/godot_bridge/vehicle_depth_staging.hpp','invalidate'),
 ('src/platform/godot_bridge/vehicle_depth_staging.hpp','lookup'),
 ('src/platform/godot_bridge/vehicle_depth_staging.hpp','publish'),
 ('src/platform/godot_bridge/vehicle_depth_staging.hpp','registry'),
 ('src/platform/godot_bridge/vehicle_depth_staging.hpp','stats'),
 ('src/platform/godot_bridge/track_submission_cache.hpp','acquire'),
 ('src/platform/godot_bridge/track_submission_cache.hpp','cached_surfaces'),
 ('src/platform/godot_bridge/track_submission_cache.hpp','clear'),
 ('src/platform/godot_bridge/track_submission_cache.hpp','dynamic_plan'),
 ('src/platform/godot_bridge/track_submission_cache.hpp','same_state'),
 ('src/platform/godot_bridge/track_submission_cache.hpp','same_submission_plan_input'),
 ('src/platform/godot_bridge/track_submission_cache.hpp','surface'),
 ('src/platform/godot_bridge/profile_debug_ui.cpp','ProfileDebugUi::draw'),
 ('src/platform/godot_bridge/profile_debug_ui.cpp','compact_sample'),
 ('src/platform/godot_bridge/profile_debug_ui.cpp','compact_graph'),
 ('src/platform/godot_bridge/ps2_render_host.cpp','Ps2RenderHost::console_frame'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','impl_'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::update'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::complete_readback'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::scheduled_viewports'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::stats'),
 ('src/platform/godot_bridge/track_sun_renderer.cpp','TrackSunRenderer::clear'),
}
# Port-only preparation/scheduling and diagnostic additions; no recovered-body scope.
ALLOWED |= {
 ('src/platform/godot_bridge/ps2_track_renderer.cpp','requested_render_thread'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::finish_view'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::note_source_upload'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::preparation_stats'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::prepare_view'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::set_cpu_source'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','TrackMeshBatchCache::source_arrays'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','key_of'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','limit'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','ms'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','pack'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','run'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','snapshot'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp','snapshot_arrays'),
 ('src/platform/godot_bridge/track_mesh_batches_tests.cpp','test_track_mesh_batch_cache'),
 ('src/platform/godot_bridge/track_submission_digest.hpp','flag'),
 ('src/platform/godot_bridge/track_submission_digest.hpp','integer'),
 ('src/platform/godot_bridge/track_submission_digest.hpp','scalar'),
 ('src/platform/godot_bridge/track_submission_digest.hpp','track_submission_digest'),
 ('src/platform/render/mesh_prepare.hpp','prepare_mesh'),
}
EXTRA='reverse/scripts/audit_task0071_review.py\nreverse/scripts/audit_task0072_review.py\nreverse/task0072/host-rendering.md\nreverse/tests/test_task0072_source_review.py\ngodot/scripts/debug/profile_debug_contract_smoke.gd\ngodot/scripts/runtime/task0072_sun_counter_smoke.gd\ngodot/scenes/task0072_sun_counter_smoke.tscn\ngodot/shaders/sun_counter.gdshaderinc'.splitlines()
AUXILIARY=predecessor.AUXILIARY|set(EXTRA)|{
    'godot/scripts/runtime/task0072_sun_transport_smoke.gd',
    'godot/scenes/task0072_sun_transport_smoke.tscn',
    'tools/task0072_sun_compare.py',
    'godot/scripts/runtime/task0072_sun_telemetry_smoke.gd',
    'godot/scenes/task0072_sun_telemetry_smoke.tscn',
    'godot/scripts/runtime/track_performance_viewer.gd',
    'godot/scripts/debug/task0072_profile_cache_smoke.gd',
    'tools/task0072_performance.py',
    'reverse/tests/test_task0072_performance.py',
    'reverse/task0072/sun.md',
    'godot/scripts/debug/task0072_profile_upload_smoke.gd',
    'godot/scenes/task0072_profile_upload_smoke.tscn',
    'CMakePresets.json',
    'godot/scripts/debug/task0072_profile_export_smoke.gd',
    'src/platform/godot_bridge/track_mesh_batches.hpp',
    'src/platform/godot_bridge/track_postprocess.hpp',
    'src/platform/godot_bridge/vehicle_depth_staging.hpp',
    'src/platform/godot_bridge/parameter_staging_registry.hpp',
    'src/tests/parameter_staging_registry_tests.cpp',
    'reverse/task0072/residency.md',
    'godot/scripts/debug/task0072_driving_harness_smoke.gd',
}


AUXILIARY |= {
 'src/platform/render/mesh_prepare.hpp',
 'src/platform/godot_bridge/track_submission_digest.hpp',
 'src/tests/mesh_prepare_tests.cpp',
 'src/tests/track_runtime_tests.cpp',
 'src/tests/track_submission_digest_tests.cpp',
 'tools/run_task0072_worker_parity.ps1',
 'tools/run_task0072_worker_matrix.ps1',
 'tools/task0072_driving_compare.py',
 'reverse/tests/test_task0072_workers.py',
 'godot/scripts/runtime/task0072_driving_parity.gd',
 'godot/scripts/debug/task0072_worker_config_smoke.gd',
}

# E0072H01 CPU submission phase: individually reviewed port-only identities.
# The old local mesh callback moved to platform/render/preparation_work.hpp.
ALLOWED.discard(('src/platform/godot_bridge/track_mesh_batches.cpp','run'))
ALLOWED |= {
 ('src/platform/godot_bridge/parameter_staging_registry.hpp', 'read_snapshot'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp', 'Ps2TrackRenderer::_notification'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp', 'Ps2TrackRenderer::depth_source_order_changed'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp', 'Ps2TrackRenderer::depth_source_structure_changed'),
 ('src/platform/godot_bridge/ps2_track_renderer.cpp', 'submit_transform'),
 ('src/platform/godot_bridge/track_mesh_batches.cpp', 'TrackMeshBatchCache::submit_math'),
 ('src/platform/godot_bridge/track_postprocess.cpp', 'TrackPostprocess::forget_material'),
 ('src/platform/godot_bridge/track_postprocess.cpp', 'TrackPostprocess::forget_mesh'),
 ('src/platform/godot_bridge/track_postprocess.cpp', 'TrackPostprocess::mark_mesh_changed'),
 ('src/platform/godot_bridge/track_postprocess.cpp', 'TrackPostprocess::needs_structure_observer'),
 ('src/platform/godot_bridge/track_postprocess.cpp', 'TrackPostprocess::notify_scene_node'),
 ('src/platform/godot_bridge/track_postprocess.cpp', 'TrackPostprocess::notify_source_order'),
 ('src/platform/godot_bridge/track_postprocess.cpp', 'TrackPostprocess::set_structure_observer'),
 ('src/platform/godot_bridge/vehicle_depth_staging.hpp', 'read_snapshot'),
 ('src/platform/render/depth_sync.hpp', 'changes'),
 ('src/platform/render/depth_sync.hpp', 'consumed'),
 ('src/platform/render/depth_sync.hpp', 'discovered'),
 ('src/platform/render/depth_sync.hpp', 'forget'),
 ('src/platform/render/depth_sync.hpp', 'mark'),
 ('src/platform/render/depth_sync.hpp', 'observe_structure'),
 ('src/platform/render/depth_sync.hpp', 'structure_changed'),
 ('src/platform/render/preparation_work.hpp', 'configure_math'),
 ('src/platform/render/preparation_work.hpp', 'rethrow_errors'),
 ('src/platform/render/preparation_work.hpp', 'run'),
 ('src/platform/render/preparation_work.hpp', 'tasks'),
 ('src/platform/render/submit_math.hpp', 'prepare_submit_math'),
 ('src/platform/render/submit_math.hpp', 'same_submit_transform'),
 ('src/platform/render/submit_math.hpp', 'submit_dot'),
 ('src/platform/render/submit_math.hpp', 'submit_inverse'),
 ('src/platform/render/submit_math.hpp', 'submit_xform'),
 ('src/platform/render/submit_math.hpp', 'submit_xform_basis'),
}
AUXILIARY |= {
 'src/platform/render/submit_math.hpp',
 'src/platform/render/preparation_work.hpp',
 'src/platform/render/depth_sync.hpp',
 'src/tests/submit_math_tests.cpp',
 'src/tests/depth_sync_tests.cpp',
 'tools/analyze_cpu_submission.py',
 'reverse/tests/test_cpu_submission_analysis.py',
}

def historical_identities():
    require(fingerprint(predecessor.REVIEW)==PREDECESSOR_REVIEW_SHA256,'Frozen TASK0071 review changed')
    require(fingerprint(predecessor.SCOPE)==PREDECESSOR_SCOPE_SHA256,'Frozen TASK0071 scope changed')
    review=json.loads(predecessor.REVIEW.read_text())
    result=transition.reviewed_identities(predecessor.historical_identities(),review)
    auxiliary={r['path']:r['lf_sha256'] for r in review['auxiliary_sources']}
    require(predecessor.check_current(current_identities=result,auxiliary_fingerprints=auxiliary)['passed'],'Historical predecessor failed')
    return result


def _slot(row):
    return row['path'],row['name']


def _annotation():
    return dict(kind='port_adapter',evidence_id='E0072H01',evidence_record=EVIDENCE,
                reason='Port-side Sun GPU transport, profiler/export/UI transport, Debug host compilation, independently switched submission/depth/staging/capture/residency candidates and visibility/payload diagnostics; recovered algorithms remain unchanged; runtime equivalence remains unverified.')


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
    baseline=ROOT/'local/task0072/baseline/source-identities.json'
    require(baseline.exists(),'Missing preserved TASK0072 startup inventory')
    require(Counter(identities._identity_key(r)+(bool(r.get('stub')), ) for r in json.loads(baseline.read_text()))==Counter(identities._identity_key(r)+(bool(r.get('stub')), ) for r in before),'Startup inventory differs from predecessor')
    current=identities.source_identities(identities.historical.collect_current())
    sets=[Counter(identities._identity_key(r) for r in rows) for rows in (before,current)]
    changed=[]
    for index,rows in enumerate((before,current)):
        delta=sets[index]-sets[1-index]
        changed.append([dict(row,**_annotation()) for row in rows if delta[identities._identity_key(row)]])
    slots={_slot(row) for rows in changed for row in rows}
    require(slots==ALLOWED,'Unreviewed identity changes: '+str({'unexpected':sorted(slots-ALLOWED),'missing':sorted(ALLOWED-slots)}))
    require(all(not r.get('stub') for rows in changed for r in rows),'Stub change is outside host scope')
    scope=dict(task='TASK-0072',entries=[dict(path=path,name=name,**_annotation()) for path,name in sorted(slots)],
               auxiliary_sources=[dict(path=path,**_annotation()) for path in sorted(AUXILIARY)])
    SCOPE.write_text(json.dumps(scope,indent=2)+'\n',encoding='utf-8')
    review=dict(task='TASK-0072',status='approved',target_binary_sha256=identities.TARGET_SHA256,
                predecessor_review_lf_sha256=PREDECESSOR_REVIEW_SHA256,
                baseline_inventory_sha256=identities.inventory_digest(before),scope_lf_sha256=fingerprint(SCOPE),
                before=changed[0],after=changed[1],
                auxiliary_sources=[dict(path=path,lf_sha256=fingerprint(ROOT/path),**_annotation()) for path in sorted(AUXILIARY)])
    REVIEW.write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
    return check_current(current_identities=current)


def check_current(*,current_identities=None,auxiliary_fingerprints=None):
    if current_identities is None and auxiliary_fingerprints is None:
        from audit_task0075_review import check_current as latest
        return latest()
    require(auxiliary_fingerprints is None or current_identities is not None,'Historical auxiliary state requires explicit identities')
    require(auxiliary_fingerprints is None or set(auxiliary_fingerprints)==AUXILIARY,'Wrong auxiliary snapshot paths')
    before=historical_identities();review=json.loads(REVIEW.read_text());scope=json.loads(SCOPE.read_text())
    require(review.get('task')==scope.get('task')=='TASK-0072' and review.get('status')=='approved','Wrong task/approval')
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
            require(all(row.get(k)==v and slots[_slot(row)].get(k)==v for k,v in _annotation().items()),'Function evidence mismatch')
    require('E0072H01' in (ROOT/EVIDENCE).read_text(),'Missing host evidence')
    auxiliary={r['path']:r for r in scope['auxiliary_sources']}
    require(set(auxiliary)==AUXILIARY and len(auxiliary)==len(scope['auxiliary_sources']),'Wrong auxiliary scope')
    require(len(review['auxiliary_sources'])==len(AUXILIARY) and {r['path'] for r in review['auxiliary_sources']}==AUXILIARY,'Wrong auxiliary review')
    for row in review['auxiliary_sources']:
        require(all(row.get(k)==v and auxiliary[row['path']].get(k)==v for k,v in _annotation().items()),'Auxiliary evidence mismatch')
        actual=fingerprint(ROOT/row['path']) if auxiliary_fingerprints is None else auxiliary_fingerprints.get(row['path'])
        require(actual==row.get('lf_sha256'),'Changed auxiliary source '+row['path'])
    current=identities.source_identities(identities.historical.collect_current()) if current_identities is None else current_identities
    amendment=identities.check_identity_review(before,current,review)
    pins={identities._identity_key(r):bool(r.get('stub')) for r in transition.reviewed_identities(before,review)}
    require(all(identities._identity_key(r) not in pins or pins[identities._identity_key(r)]==bool(r.get('stub')) for r in current),'Changed stub classification')
    return dict(passed=amendment['passed'],task0072_amendment=amendment)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write',action='store_true',help='Explicitly pin the settled allowlisted host changes')
    parser.add_argument('--delta',action='store_true',help='Inspect the unapproved current delta without writing pins')
    args=parser.parse_args()
    require(not (args.write and args.delta),'Choose either --write or --delta')
    result=inspect_delta() if args.delta else write_review() if args.write else check_current()
    print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
