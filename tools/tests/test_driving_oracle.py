"""TASK-0038 safety/consistency boundaries; no emulator or original assets required."""
import struct
import unittest
from tools.driving_oracle import read_block,capture,WORLD

class FakePine:
    def __init__(self,status="paused",game="SLUS-20362",world=0):
        self.state,self.game,self.world=status,game,world
        self.calls=[]
    def status(self):return self.state
    def request(self,payload):
        self.calls.append(payload)
        if payload==b"\x0c":
            data=self.game.encode()+b"\0";return struct.pack("<I",len(data))+data
        if any(payload[i]!=2 for i in range(0,len(payload),5)):
            raise AssertionError("Read helper attempted a non-read command")
        return b"\0"*(len(payload)//5*4)
    def read32(self,address):
        return {0x137c88:0x3c013f80,0x137d90:0x27bdff00,WORLD:self.world}.get(address,0)

class DrivingOracleTests(unittest.TestCase):
    def test_rejects_running_before_memory_reads(self):
        pine=FakePine(status="running")
        with self.assertRaisesRegex(RuntimeError,"Pause PCSX2"):capture(pine,0)
        self.assertEqual(pine.calls,[])
    def test_rejects_wrong_game_and_null_world(self):
        with self.assertRaisesRegex(RuntimeError,"Wrong game"):capture(FakePine(game="OTHER"),0)
        with self.assertRaisesRegex(RuntimeError,"No active World"):capture(FakePine(),0)
    def test_only_reads_bounded_aligned_ee_memory(self):
        pine=FakePine()
        self.assertEqual(read_block(pine,0x100000,8),b"\0"*8)
        self.assertEqual(pine.calls,[b"\x02"+struct.pack("<I",0x100000)+b"\x02"+struct.pack("<I",0x100004)])
        for address,size in [(1,4),(0,3),(0x2000000,4),(0,0),(0,0x10004)]:
            with self.assertRaises(ValueError):read_block(pine,address,size)
    def test_truncated_reply_is_not_a_snapshot(self):
        class Truncated(FakePine):
            def request(self,payload):return b"\0"*3
        with self.assertRaisesRegex(ValueError,"reply length"):read_block(Truncated(),0x100000,4)

if __name__=="__main__":unittest.main()
