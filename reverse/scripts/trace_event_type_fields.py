"""TASK-0043 field identity gates for EventInfo host projections.

Names originate in SHA-bound STABS; Retail offsets originate in independently
adopted exact-leaf getters, inspected through live read-only Ghidra.
"""
import csv
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "reverse/scripts"))
from ghidra import Client
from extract_debug_reference_types import EXPECTED_SHA, local_strings, declarations, parse_body


def main():
    raw = Path("D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF").read_bytes()
    assert hashlib.sha256(raw).hexdigest() == EXPECTED_SHA
    offset, table = local_strings(raw)
    records = {}
    for file_offset, match in declarations(table, offset):
        name, _, _, size, body = match.groups()
        if name not in ("EventInfo", "EventTree"):
            continue
        fields, _, _ = parse_body(body)
        records.setdefault(name, []).append(dict(file_offset=hex(file_offset), size=int(size), fields={n:o//8 for n,o,s in fields}))
    mapping = {r["retail_address"]: r for r in csv.DictReader((ROOT / "reverse/naming/task0042-adopted-retail-names.csv").open())}
    out = ROOT / "local/task0043/title"
    out.mkdir(parents=True, exist_ok=True)
    client = Client()
    entries = [
        ("getgoal", "0x001FBC08", "0x002008B8", "nGoals", 0x3D, "aGoals", 0x7C, "pGoals", 0x24),
        ("getcar", "0x001FBBC8", "0x00200878", "nCars", 0x3B, "aCars", 0x44, "pCars", 0x14),
        ("gettrack", "0x001FBC48", "0x002008F8", "nTracks", 0x3C, "aTracks", 0x6C, "pTracks", 0x1C),
        ("getreward", "0x001FBC88", "0x00200938", "nRewards", 0x3E, "aRewards", 0x8C, "pRewards", 0x2C),
    ]
    assert mapping["0x001FFF60"]["debug_address"] == "0x001FB458"
    for prefix, program, address in (("debug", "NFSHP2.ELF", "0x001FB458"), ("retail", "SLUS_203.62", "0x001FFF60")):
        query = dict(file_name=program, action="decompile", address=address)
        result = client.call("tools/call", dict(name="inspect", arguments=query))
        assert not result.get("isError")
        (out / (prefix + "-setevents.json")).write_text(json.dumps(dict(query=query, result=result), indent=2), encoding="utf-8")
    old_fields = {"nGoals":"link_count_0x3D", "aGoals":"link_index_0x7C", "nCars":"Field_0x3B", "aCars":"Field_0x44", "nTracks":"Field_0x3C", "aTracks":"Field_0x6C", "nRewards":"Field_0x3E", "aRewards":"Field_0x8C"}
    bridges=[]
    for stem, da, ra, count, co, array, ao, pointer, po in entries:
        assert mapping[ra]["debug_address"] == da
        for prefix, program, address in (("debug", "NFSHP2.ELF", da), ("retail", "SLUS_203.62", ra)):
            query=dict(file_name=program, action="decompile", address=address)
            result=client.call("tools/call", dict(name="inspect", arguments=query))
            assert not result.get("isError")
            (out/(prefix+"-"+stem+".json")).write_text(json.dumps(dict(query=query,result=result),indent=2),encoding="utf-8")
        for original, byte_offset in ((count,co),(array,ao)):
            variants=[v for v in records["EventInfo"] if v["fields"].get(original)==byte_offset and v["size"]==188]
            assert variants
            bridges.append(dict(entity_kind="field", owner="EventRecord_001FFF60", original_owner="EventInfo", old_identifier=old_fields[original], recovered_name=original, retail_offset=hex(byte_offset), debug_offset=hex(byte_offset), retail_function=ra, debug_function=da, correspondence_evidence=mapping[ra]["evidence_id"], stabs_file_offset=variants[0]["file_offset"], evidence_id="E-0043-F01", status="supported_same_field_in_independently_mapped_getter"))
        assert any(v["fields"].get(pointer)==po for v in records["EventTree"])
        if pointer != "pGoals":
            bridges.append(dict(entity_kind="field", owner="EventTree_001FFC40", original_owner="EventTree", old_identifier="Field_0x"+format(po,"02X"), recovered_name=pointer, retail_offset=hex(po), debug_offset=hex(po), retail_function=ra, debug_function=da, correspondence_evidence=mapping[ra]["evidence_id"], evidence_id="E-0043-F01", status="supported_same_table_in_independently_mapped_getter", caveat="Host vector projects original table pointer; native layout is not asserted."))
    result=dict(schema_version=1,evidence_id="E-0043-F01",retail_sha256="7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2",debug_sha256=EXPECTED_SHA,bridges=bridges,type_findings=[dict(old_identifier="EventRecord_001FFF60", recovered_name="EventInfo",status="supported_record_identity",evidence="Independently mapped SetEvents 001FFF60 / 001FB458 stores pEvents and walks 188-byte records; independently mapped EventInfo getters operate on the same record fields. Host storage remains projected.")],unknowns=["No whole-type promotion for EventTree_001FFC40 is proposed here.","No field name is transferred from an uncorrelated Debug offset.","Other EventInfo fields are not promoted solely because offsets coincide."], raw_directory="local/task0043/title",verification_status="not_tested")
    target=ROOT/"reverse/naming/task0043-type-field-bridges.json"
    target.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(dict(bridges=len(bridges),result="PASS",output=str(target))))


if __name__ == "__main__":
    main()
