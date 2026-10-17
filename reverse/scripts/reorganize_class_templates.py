"""TASK-0042 one-time evidence-backed standalone-shell migration.

The durable class layout registry is authoritative to the regular generator.
Existing native bodies are copied byte-for-byte apart from include-path updates.
Only sole-throw inherited member shells are removed from nondeclaring classes.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re

import generate_class_templates as generator

ROOT = generator.ROOT


def migrate():
    layout = json.loads((ROOT / 'reverse/classes/task0042-class-layout.json').read_text())
    inventory = json.loads((ROOT / 'reverse/classes/template-inventory.json').read_text())
    old_classes = {c['name']: c for c in inventory['classes']}
    contents, destinations, include_map = {}, {}, {}
    for cls in layout['classes'].values():
        for key in ('source', 'header'):
            old, target = cls['old_' + key], cls[key]
            if not old or old == target:
                continue
            destinations[old] = target
            if key == 'header':
                include_map[old.removeprefix('src/')] = target.removeprefix('src/')
    for old in destinations:
        path = ROOT / old
        if not path.exists():
            raise ValueError('One-time migration prerequisite missing: ' + old)
        contents[old] = path.read_text(encoding='utf-8')
    for transfer in layout['canonical_members']:
        address = transfer['elf_address']
        for name in transfer['remove_from']:
            cls = old_classes[name]
            member = next(m for m in cls['members'] if m['elf_address'] == address)
            for key in ('source', 'header'):
                path = cls[key]
                if path not in contents:
                    contents[path] = (ROOT / path).read_text(encoding='utf-8')
                text = contents[path]
                method = re.escape(member['method'])
                if key == 'header':
                    pattern = (r'(?m)^    // ELF ' + re.escape(address) +
                        r';[^\n]*\n(?:(?:    //[^\n]*|\s*)\n)*    void ' + method + r'\(\);\n')
                    text, count = re.subn(pattern, '', text)
                    if count != 1:
                        raise ValueError(f'Cannot safely remove declaration {name} {address}: {count}')
                else:
                    masked = generator.mask_comments_strings(text)
                    matches = [m for m in generator.METHOD.finditer(masked)
                        if re.search(r'\b' + re.escape(name) + r'::' + method + r'\s*\(\s*\)', m[1])]
                    if len(matches) != 1:
                        raise ValueError(f'Cannot identify sole whole-entry definition {name} {address}')
                    match = matches[0]
                    end = generator.closing_brace(masked, match.end() - 1)
                    body = text[match.end():end]
                    clean = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/', '', body).strip()
                    if not re.fullmatch(r'hp2::not_implemented\("' + re.escape(address) + r'",\s*"[^"\n]*"\);', clean):
                        raise ValueError(f'Native body requires manual transfer: {name} {address}')
                    start = match.start()
                    previous = text.rfind('\n', 0, start - 1) + 1
                    if text[previous:start].startswith('// ELF ' + address):
                        start = previous
                    text = text[:start] + text[end + 1:]
                contents[path] = text
    outputs = {}
    for old, text in contents.items():
        target = destinations.get(old, old)
        for src, dest in include_map.items():
            text = text.replace('"' + src + '"', '"' + dest + '"')
        if target not in outputs:
            # Case-only paths refer to the same Windows file, so do not append twice.
            moved_here = any(p.casefold() == target.casefold() for p in destinations)
            path = ROOT / target
            outputs[target] = '' if moved_here or target in contents or not path.exists() else path.read_text(encoding='utf-8')
        outputs[target] += ('\n' if outputs[target] else '') + text
    for target, text in outputs.items():
        generator.write_text(target, text)
    for old, target in destinations.items():
        if old.casefold() != target.casefold() and old not in outputs:
            (ROOT / old).unlink()
        elif old != target and old.casefold() == target.casefold():
            temporary = (ROOT / old).with_name(Path(old).name + '.task0042-rename')
            (ROOT / old).rename(temporary)
            temporary.rename(ROOT / target)
    # A header can move while its owning CPP stays at its original port partition.
    for cls in old_classes.values():
        for key in ('source', 'header'):
            path = destinations.get(cls[key], cls[key])
            if path in outputs or not (ROOT / path).exists():
                continue
            text = (ROOT / path).read_text(encoding='utf-8')
            for src, dest in include_map.items():
                text = text.replace('"' + src + '"', '"' + dest + '"')
            generator.write_text(path, text)
    print(f'CLASS_SHELL_MIGRATION_OK: {len(destinations)} file mappings, {len(layout["canonical_members"])} canonical member groups')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', required=True, action='store_true')
    parser.parse_args()
    migrate()
