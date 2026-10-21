"""Observe a TASK0053 source delta. This command cannot create an approved review."""
from collections import Counter
import argparse,json
from pathlib import Path
import audit_task0053_review as audit

def collect_delta(before,current):
    key=audit.identities._identity_key
    def difference(rows,other):
        pending=Counter(key(row) for row in rows)-Counter(key(row) for row in other)
        result=[]
        for row in rows:
            identity=key(row)
            if pending[identity]:result.append(dict(row));pending[identity]-=1
        return result
    return difference(before,current),difference(current,before)

def protected_output(path):
    frozen={Path(name).resolve() for name,digest in audit._chain_files()}
    return path.resolve() in frozen|{audit.REVIEW.resolve(),audit.SCOPE.resolve()}

def report():
    before=audit.historical_identities()
    current=audit.identities.source_identities(audit.identities.historical.collect_current())
    removed,added=collect_delta(before,current)
    scope=json.loads(audit.SCOPE.read_text());slots={(r['path'],r['name']):r for r in scope.get('entries',[])}
    unmapped=[]
    for row in removed+added:
        slot=slots.get((row['path'],row['name']))
        if slot is None:unmapped.append(dict(path=row['path'],name=row['name']))
        else:row.update({k:v for k,v in slot.items() if k not in ('path','name')})
    draft=dict(task='TASK-0053',status='draft-not-approval',target_binary_sha256=audit.identities.TARGET_SHA256,
        predecessor_review_lf_sha256=audit.PREDECESSOR_REVIEW_SHA256,
        baseline_inventory_sha256=audit.identities.inventory_digest(before),scope_lf_sha256=audit.fingerprint(audit.SCOPE),
        before=removed,after=added,unmapped=unmapped)
    draft['validation_errors']=audit.scope_errors(draft,current)
    return draft

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path)
    args=parser.parse_args();result=report()
    output=json.dumps(result,indent=2)+'\n'
    if args.report:
        if protected_output(args.report):parser.error('Cannot overwrite approved review, scope or frozen predecessor inputs; observation is draft-only')
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(output,encoding='utf-8')
    print(json.dumps(dict(status=result['status'],before=len(result['before']),after=len(result['after']),unmapped=result['unmapped'],validation_errors=result['validation_errors']),indent=2))
    raise SystemExit(1 if result['validation_errors'] or result['unmapped'] else 0)
