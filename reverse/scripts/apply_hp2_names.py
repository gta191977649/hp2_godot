"""Journaled conservative Ghidra name apply/rollback. No analysis or prototype mutation."""
from hp2_naming import *
import argparse

def execute(api, changes, journal, persist, reverse=False):
    """Name-only batch with live preconditions and rollback of completed/pending writes."""
    completed = []
    try:
        for item in changes:
            old, new = (item['new_name'], item['old_name']) if reverse else (item['old_name'], item['new_name'])
            if api.get(item['address']) != old:
                raise ValueError('Live name differs from precondition')
            event = dict(address=item['address'], old_name=old, new_name=new, status='pending')
            journal.append(event); persist(journal)
            completed.append(event)
            api.rename(item['address'], new)
            if api.get(item['address']) != new:
                raise RuntimeError('Rename not confirmed')
            event['status']='applied'; persist(journal)
    except Exception:
        for event in reversed(completed):
            current = api.get(event['address'])
            if current == event['new_name']:
                api.rename(event['address'],event['old_name'])
                if api.get(event['address']) != event['old_name']:
                    raise RuntimeError('Rollback not confirmed')
                event['status']='rolled_back'
            elif current == event['old_name']:
                event['status']='unchanged'
            else:
                event['status']='rollback_blocked_external_name'
            persist(journal)
        raise

class GhidraNames:
    def __init__(self, client): self.client=client
    def get(self, address):
        return rpc(self.client,'functions',action='get',address=address)['name']
    def rename(self,address,name):
        rpc(self.client,'symbols',action='update',address=address,new_name=name)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--apply',action='store_true')
    parser.add_argument('--rollback',action='store_true')
    parser.add_argument('--finalize',action='store_true',help='Validate and save an already-applied journal after reviewing a postcondition failure')
    args=parser.parse_args()
    if sum([args.apply,args.rollback,args.finalize])!=1:
        parser.error('Choose exactly --apply, --rollback or --finalize')
    check_elf()
    plan=json.loads((PRIVATE/'rename-plan.json').read_text())
    if plan['sha256']!=SHA: raise ValueError('Plan binary mismatch')
    before=json.loads((PRIVATE/'before.json').read_text())['functions']
    client=Client(); rows=functions(client)
    if bounds(rows)!=bounds(before): raise ValueError('Function boundaries changed')
    validate_plan(rows,plan['renames'],reverse=args.rollback or args.finalize)
    # Unexpected external renames are not permitted between snapshot and operation.
    expected={r['entry_point']:r['name'] for r in before}
    if args.rollback or args.finalize:
        expected.update({r['address']:r['new_name'] for r in plan['renames']+plan['automatic_aliases']})
    if any(r['name']!=expected[r['entry_point']] for r in rows):
        raise ValueError('Unrelated Ghidra name changed; review snapshot')
    journal_path=PRIVATE/('rollback-journal.json' if args.rollback else 'apply-journal.json')
    if journal_path.exists() and not args.finalize:
        raise ValueError('Journal already exists; inspect it before retrying')
    if args.finalize:
        journal=json.loads(journal_path.read_text())
        if len(journal)!=len(plan['renames']) or any(e['status']!='applied' for e in journal):
            raise ValueError('Apply journal is not fully confirmed')
    else:
        journal=[]
        api=GhidraNames(client)
        execute(api,plan['renames'],journal,lambda j:dump(journal_path,j),reverse=args.rollback)
        if args.rollback:
            # Naming APIs cannot restore DEFAULT source type. Explicitly restore
            # alias names if the target's USER_DEFINED source alters their rendering.
            for alias in plan['automatic_aliases']:
                current=api.get(alias['address'])
                if current==alias['old_name']: continue
                target=next(c for c in plan['renames'] if c['address']==alias['target'])
                if current not in (target['old_name'],alias['new_name']):
                    raise ValueError('Unexpected alias name during rollback')
                event=dict(address=alias['address'],old_name=current,new_name=alias['old_name'],status='pending')
                journal.append(event); dump(journal_path,journal)
                api.rename(alias['address'],alias['old_name'])
                if api.get(alias['address'])!=alias['old_name']:
                    raise RuntimeError('Alias rollback not confirmed')
                event['status']='applied'; dump(journal_path,journal)
    after=functions(client)
    try:
        verify_state(before,after,plan['renames'],plan['automatic_aliases'],reverse=args.rollback)
    except ValueError as error:
        raise RuntimeError('Postcondition failed; inspect journal and use name rollback') from error
    check_elf()
    # Store actual diff including automatic thunk aliases, before persisting the project.
    rollback=[]
    old={r['entry_point']:r for r in before}
    for r in after:
        if r['name']!=old[r['entry_point']]['name']:
            rollback.append(dict(address=r['entry_point'],old_name=old[r['entry_point']]['name'],new_name=r['name']))
    dump(PRIVATE/('after-rollback.json' if args.rollback else 'after.json'),dict(sha256=SHA,functions=after))
    if not args.rollback: dump(PRIVATE/'rollback-map.json',rollback)
    saved=rpc(client,'project',action='save')
    dump(PRIVATE/('rollback-save.json' if args.rollback else 'apply-save.json'),saved)
    print(json.dumps(dict(direct=len(plan['renames']),actual_name_changes=len(rollback),rollback=args.rollback,save=saved)))

if __name__=='__main__': main()
