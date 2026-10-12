"""Synthetic safety/rollback tests; no proprietary bytes and no live Ghidra writes."""
from copy import deepcopy
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from hp2_naming import validate_plan, bounds, default_name, verify_state
from apply_hp2_names import execute

def row(addr,name):
    return dict(entry_point=addr,start_address=addr,end_address=f'{int(addr,16)+15:08x}',name=name)

def change(addr,old,new):
    return dict(address=addr,old_name=old,new_name=new,confidence='high',
        category='analysis_purpose',evidence_ids=['E-SYNTHETIC'])

class Fake:
    def __init__(self,names,fail=None,fail_after=False):
        self.names=dict(names); self.fail=fail; self.fail_after=fail_after; self.fired=False
    def get(self,a): return self.names[a]
    def rename(self,a,n):
        if a==self.fail and not self.fired:
            self.fired=True
            if self.fail_after: self.names[a]=n
            raise RuntimeError('synthetic server error')
        self.names[a]=n

class NamingTests(unittest.TestCase):
    def setUp(self):
        self.rows=[row('00001000','FUN_00001000'),row('00002000','FUN_00002000')]
        self.changes=[change('00001000','FUN_00001000','HP2_TestA'),change('00002000','FUN_00002000','HP2_TestB')]
    def test_roundtrip(self):
        fake=Fake({r['entry_point']:r['name'] for r in self.rows}); journal=[]; saved=[]
        validate_plan(self.rows,self.changes)
        execute(fake,self.changes,journal,lambda j:saved.append(deepcopy(j)))
        self.assertEqual(saved[0][0]['status'],'pending')
        after=[row(a,n) for a,n in fake.names.items()]
        validate_plan(after,self.changes,reverse=True)
        execute(fake,self.changes,[],lambda j:None,reverse=True)
        self.assertEqual(fake.names,{r['entry_point']:r['name'] for r in self.rows})
        self.assertEqual(bounds(after),bounds(self.rows))
    def test_failure_before_write_rolls_back(self):
        fake=Fake({r['entry_point']:r['name'] for r in self.rows},'00002000')
        journal=[]
        with self.assertRaises(RuntimeError): execute(fake,self.changes,journal,lambda j:None)
        self.assertEqual(fake.names,{r['entry_point']:r['name'] for r in self.rows})
        self.assertEqual(journal[0]['status'],'rolled_back')
    def test_failure_after_write_rolls_back_pending(self):
        fake=Fake({r['entry_point']:r['name'] for r in self.rows},'00002000',True)
        with self.assertRaises(RuntimeError): execute(fake,self.changes,[],lambda j:None)
        self.assertEqual(fake.names,{r['entry_point']:r['name'] for r in self.rows})
    def test_stale_live_name(self):
        fake=Fake({'00001000':'UserName','00002000':'FUN_00002000'})
        with self.assertRaises(ValueError): execute(fake,self.changes,[],lambda j:None)
        self.assertEqual(fake.get('00001000'),'UserName')
    def test_reject_unsafe_candidates(self):
        mutations=[('confidence','medium'),('evidence_ids',[]),('new_name','bad-name'),
                   ('new_name','NoPrefix'),('old_name','ExistingCustom'),('category','invented')]
        for key,value in mutations:
            with self.subTest(key=key,value=value):
                changes=deepcopy(self.changes); changes[0][key]=value
                with self.assertRaises(ValueError): validate_plan(self.rows,changes)
    def test_duplicate_names_addresses(self):
        for key in ('new_name','address'):
            changes=deepcopy(self.changes); changes[1][key]=changes[0][key]
            with self.assertRaises(ValueError): validate_plan(self.rows,changes)
    def test_name_collision(self):
        rows=self.rows+[row('00003000','HP2_TestA')]
        with self.assertRaises(ValueError): validate_plan(rows,self.changes)
    def test_boundary_guard(self):
        for end in ('00001000','00021000'):
            rows=deepcopy(self.rows); rows[0]['end_address']=end
            with self.assertRaises(ValueError): validate_plan(rows,self.changes)
    def test_default_identifier_detection(self):
        self.assertTrue(default_name('FUN_00001000'))
        self.assertFalse(default_name('HP2_Foo'))
        self.assertFalse(default_name('FUN_notanaddress'))
        self.assertFalse(default_name('thunk_FUN_00001000'))
    def test_default_thunks_render_target_name(self):
        before=self.rows+[row('00003000','thunk_FUN_00001000')]
        aliases=[dict(address='00003000',new_name='HP2_TestA')]
        after=[row('00001000','HP2_TestA'),row('00002000','HP2_TestB'),row('00003000','HP2_TestA')]
        self.assertTrue(verify_state(before,after,self.changes,aliases))
        after[2]['name']='thunk_HP2_TestA'
        with self.assertRaises(ValueError): verify_state(before,after,self.changes,aliases)
    def test_unrelated_namespace_or_name_change_rejected(self):
        for field,value in [('name','Other'),('namespace','Other')]:
            after=deepcopy(self.rows); after[0][field]=value
            with self.assertRaises(ValueError): verify_state(self.rows,after,[])

if __name__=='__main__': unittest.main()
