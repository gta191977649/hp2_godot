"""Port-side DebugServer bounds, snapshot consistency and execution cleanup tests."""
import json
import unittest
from unittest.mock import patch
from tools.pcsx2_mcp import DebugServer


class FakeServer(DebugServer):
    def __init__(self):
        super().__init__()
        self.calls=[]
        self.states=[]
        self.breakpoints=[]
        self.stopped=True
        self.fail_resume=False

    def request(self, command, **fields):
        self.calls.append((command,fields))
        if command=='status':
            if self.states:return {'ok':True,'data':self.states.pop(0)}
            return {'ok':True,'data':{'alive':True,'paused':self.stopped,'pc':'0x00137c88','cycles':1}}
        if command=='list_breakpoints':return {'ok':True,'breakpoints':self.breakpoints}
        if command=='resume' and self.fail_resume:raise ConnectionError('Lost server')
        if command=='read_memory':return {'ok':True,'hex':'00'*fields['length']}
        if command=='read_registers':return {'ok':True,'data':{'pc':'0x00137c88'}}
        return {'ok':True}


class DebugServerTests(unittest.TestCase):
    def test_memory_bounds_and_length(self):
        server=FakeServer()
        for address,size in [(-1,4),(0,0),(0,65537),(0x1fffffc,8)]:
            with self.assertRaises(ValueError):server.read_memory(address,size)
        self.assertEqual(server.calls,[])
        self.assertEqual(server.read_memory(0x100000,4),bytes(4))

    def test_running_snapshot_refused(self):
        server=FakeServer();server.stopped=False
        with self.assertRaises(RuntimeError):server.snapshot()
        self.assertEqual([c[0] for c in server.calls],['status'])

    def test_snapshot_detects_changed_cycles(self):
        server=FakeServer()
        server.states=[{'alive':True,'paused':True,'pc':'0x00137c88','cycles':c} for c in (1,2)]
        with self.assertRaises(RuntimeError):server.snapshot()

    def test_owned_breakpoint_cleanup_on_success(self):
        server=FakeServer();server.run_until(0x137c88)
        commands=[c[0] for c in server.calls]
        self.assertIn('resume',commands)
        self.assertEqual(commands[-3:],['pause','status','remove_breakpoint'])
        self.assertNotIn('step',commands)

    def test_preexisting_breakpoint_preserved(self):
        server=FakeServer();server.breakpoints=[{'address':'0x00137c88'}]
        with self.assertRaises(RuntimeError):server.run_until(0x137c88)
        self.assertEqual([c[0] for c in server.calls],['status','list_breakpoints'])

    def test_resume_failure_still_pauses_and_cleans(self):
        server=FakeServer();server.fail_resume=True
        with self.assertRaises(ConnectionError):server.run_until(0x137c88)
        self.assertEqual([c[0] for c in server.calls][-3:],['pause','status','remove_breakpoint'])

    def test_timeout_still_pauses_and_cleans(self):
        server=FakeServer()
        with patch('tools.pcsx2_mcp.time.monotonic',side_effect=[0,4,4]):
            with self.assertRaises(TimeoutError):server.run_until(0x137c88,3)
        self.assertEqual([c[0] for c in server.calls][-3:],['pause','status','remove_breakpoint'])

    def test_invalid_execution_bound_does_not_contact_server(self):
        server=FakeServer()
        for address,seconds in [(0,3),(0x137c89,3),(0x137c88,0),(0x137c88,11)]:
            with self.assertRaises(ValueError):server.run_until(address,seconds)
        self.assertEqual(server.calls,[])


if __name__=='__main__':unittest.main()
