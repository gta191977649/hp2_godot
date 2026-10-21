"""TASK-0044: prevent parallel driving and known mixed-version address regressions."""
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


class DebugDrivingChainTests(unittest.TestCase):
    def test_production_enters_the_canonical_class(self):
        source = (ROOT / 'src/game_core/port/driving/native_backend.cpp').read_text()
        self.assertIn('hp2::recovered::PhysicsCar(next,mover_).Move(timestep,services)', source)
        # TASK-0045 validates the catalogue before committing the loaded image.
        self.assertIn('auto image=read_debug_image(elf_)', source)
        self.assertIn('auto catalog=read_surface_catalog(image)', source)
        self.assertIn('image_=std::move(image)', source)
        # TASK-0048 wraps the same canonical Move for contact-time backup/re-move.
        self.assertIn('DynamicCollisionBody::BeginMove(next,body,timestep)', source)
        self.assertIn('next_collisions.process(next,body,car_,track->topology,motion,move,recompute)', source)
        self.assertNotIn('native::Move(', source)
        self.assertNotIn('read_original_image(', source)
        bridge = (ROOT / 'src/platform/godot_bridge/ps2_driving_runtime.cpp').read_text()
        self.assertIn('port::driving::debug_image_path(', bridge)
        wrapper = (ROOT / 'src/platform/godot_bridge/ps2_driving_lab.cpp').read_text()
        header = (ROOT / 'src/platform/godot_bridge/ps2_driving_lab.hpp').read_text()
        self.assertIn('class Ps2DrivingLab : public Ps2DrivingRuntime', header)
        self.assertIn('HP2_DEBUG_BUILD != 0', header)
        self.assertNotIn('NativeBackend', wrapper)
        self.assertNotIn('session_', wrapper)

    def test_composite_solver_has_one_owner(self):
        for name in ('projections.hpp', 'unresolved_entries.cpp'):
            source = (ROOT / 'src/game_core/port/driving' / name).read_text()
            self.assertNotIn('struct SolverDependencies', source)
            self.assertNotRegex(source, r'IterateForces\(SolverBodyProjection')
        core = (ROOT / 'src/game_core/indep/src/physicscar.cpp').read_text()
        self.assertEqual(len(re.findall(r'void PhysicsCar::IterateForces\(\)', core)), 1)

    def test_player_and_skid_paths_use_original_owners(self):
        backend = (ROOT / 'src/game_core/port/driving/native_backend.cpp').read_text()
        self.assertLess(backend.index('driver.Drive(dt,player)'), backend.index('PhysicsCar(next,mover_).Move(timestep,services)'))
        # Output arbitration belongs to RealDriver, never to the tool backend.
        self.assertNotRegex(backend, r'next\.(?:scalar|word|half)\(controls_\+0x21[048c],')
        player = (ROOT / 'src/game_core/indep/src/realdriver.cpp').read_text()
        self.assertEqual(1, len(re.findall(r'void RealDriver::Drive\(', player)))
        self.assertEqual(1, len(re.findall(r'void RealDriver::DoThrottleBrakeReverse\(', player)))
        wheel = (ROOT / 'src/game_core/indep/src/wheel.cpp').read_text()
        self.assertIn('SkidMaker', wheel)
        self.assertIn('hp2::recovered::RenderSkids(candidate,view,services)', backend)
        skids = (ROOT / 'src/game_core/indep/src/skids.cpp').read_text()
        self.assertEqual(1, len(re.findall(r'void SkidMaker::MakeSkid\(', skids)))
        self.assertEqual(1, len(re.findall(r'void RenderSkids\(', skids)))

    def test_successor_owners_are_separate_and_generator_protected(self):
        import sys
        sys.path.insert(0, str(ROOT / 'reverse/scripts'))
        import generate_class_templates as generator
        registry = json.loads((ROOT / 'reverse/classes/task0046-debug-canonical-owners.json').read_text())
        owners = generator.debug_canonical_owners()
        for name, entry in registry['classes'].items():
            self.assertEqual(entry['header'], owners[name]['header'])
            self.assertEqual('reverse/classes/task0046-debug-canonical-owners.json', owners[name]['registry_source'])
        for name in ('RealDriver', 'CarDriver', 'SkidSegment', 'ePoly'):
            self.assertIn(name, registry['classes'])

    def test_global_adapter_matches_reviewed_bindings_and_rejects_unknowns(self):
        rows = json.loads((ROOT / 'reverse/task0044/debug-global-bindings.json').read_text())['entries']
        expected = {int(r['retail_address'], 16): int(r['debug_address'], 16) for r in rows}
        self.assertEqual(len(expected), len(rows))
        source = (ROOT / 'src/game_core/port/driving/debug_globals.hpp').read_text()
        actual = {int(a, 16): int(b, 16) for a, b in re.findall(r'case (0x\w+)u: return (0x\w+)u;', source)}
        self.assertEqual(expected, actual)
        self.assertIn('default: hp2::not_implemented(', source)

    def test_known_retail_globals_cannot_be_read_directly(self):
        rows = json.loads((ROOT / 'reverse/task0044/debug-global-bindings.json').read_text())['entries']
        retired = {int(r['retail_address'], 16) for r in rows}
        paths = [ROOT / 'src/game_core/port/driving/adapters.cpp',
                 ROOT / 'src/game_core/port/driving/native_backend.cpp']
        paths += [ROOT / ('src/game_core/indep/src/' + name + '.cpp') for name in
                  ('physicscar', 'engine', 'wheel', 'suspension', 'rigidbody', 'componentphysics')]
        for path in paths:
            source = path.read_text()
            for address in re.findall(r'\b(?:m|state|next)\.(?:word|scalar|vector|matrix|copy)\(\s*(0x[0-9a-fA-F]+)', source):
                self.assertNotIn(int(address, 16), retired, (path.name, address))


if __name__ == '__main__':
    unittest.main()
