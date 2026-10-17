"""Derive current projection bindings from TASK-0042 plus approved name changes.

All expected token hashes come from the fixed pre-change Git revision, not the
working code being tested. The historical TASK-0042 ledger remains unchanged.
"""
import argparse
import json
import re
import subprocess
from pathlib import Path
from adopt_task0043_identifiers import ROOT, RENAMES, token_replace
from reorganize_driving_projections import functions, cpp_tokens_sha256

BASELINE = '19a8de92af90788959ddcc5bcae1d0c4367fb107'


def derive():
    ledger=json.loads((ROOT/'reverse/naming/task0042-projection-partitions.json').read_text())
    expected={}
    def original(path):
        if path not in expected:
            expected[path]=subprocess.check_output(['git','show',BASELINE+':'+path],cwd=ROOT).decode('utf-8')
        return expected[path]
    for row in ledger['functions']:
        text=original(row['source'])
        fs,_=functions(text)
        hits=[f for f in fs if f['scope']==row['source_scope'] and
              ' '.join(f['signature'].split())==' '.join(row['source_signature'].split())]
        if len(hits)!=1:
            raise ValueError('Ambiguous baseline projection: '+str(row))
        f=hits[0]
        before=re.sub(r'\n    using namespace detail_[A-Za-z_0-9]+;', '',text[f['start']:f['end']])
        if cpp_tokens_sha256(before)!=row['migrated_tokens_sha256']:
            raise ValueError('Historical TASK-0042 token mismatch')
        row['predecessor_signature']=row['source_signature']
        row['predecessor_tokens_sha256']=row['migrated_tokens_sha256']
        row['source_signature']=token_replace(row['source_signature'],RENAMES)
        row['source_identifier']=RENAMES.get(row['source_identifier'],row['source_identifier'])
        row['migrated_tokens_sha256']=cpp_tokens_sha256(token_replace(before,RENAMES))
    support=original('src/game_core/port/driving/projection_support.hpp')
    _,ns=functions(support)
    for row in ledger['helper_chunks']:
        hits=[n for n in ns if n[3]==row['scope'] and
              cpp_tokens_sha256(support[n[1]:n[2]])==row['migrated_tokens_sha256']]
        if len(hits)!=1:
            raise ValueError('Ambiguous historical helper chunk')
        n=hits[0]
        row['predecessor_tokens_sha256']=row['migrated_tokens_sha256']
        row['migrated_tokens_sha256']=cpp_tokens_sha256(token_replace(support[n[1]:n[2]],RENAMES))
    ledger.update(task_id='TASK-0043',baseline_commit=BASELINE,
        predecessor='reverse/naming/task0042-projection-partitions.json',
        rename_evidence='reverse/naming/task0043-identifier-renames.json',
        policy='Current path/signature bindings and expected token hashes derive only from the historical ledger, fixed Git baseline and reviewed identifier substitutions. No working-body hashes are accepted as evidence of preservation.')
    return ledger


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--write',action='store_true')
    args=p.parse_args()
    target=ROOT/'reverse/naming/task0043-projection-bindings.json'
    expected=json.dumps(derive(),indent=2)+'\n'
    if args.write:
        target.write_text(expected)
    elif not target.exists() or target.read_text()!=expected:
        raise ValueError('Stale TASK-0043 projection bindings')
    print('CURRENT_PROJECTION_BINDINGS_OK: 151 functions, 12 helper chunks')


if __name__=='__main__':
    main()
