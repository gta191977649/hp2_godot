# EventInfo and EventTree field-name bridges

- Status: Identified (names only; no native layout verification).
- Evidence: E-0043-F01; independently mapped E-0041-M01 getters and E-0042-S001 ownership.
- Retail SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Debug SHA-256: 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.

## Confirmed

EventRecord_001FFF60's authored 188-byte records are EventInfo, established by
SetEvents and the independently mapped EventInfo GetCar/GetTrack/GetGoal/GetReward
access paths. Eight record field names and three EventTree table names are
recovered verbatim; exact offsets and both-version functions are recorded in
reverse/naming/task0043-type-field-bridges.json. Live symbols/decompilations and
SHA-bound STABS corroborate the same fields; raw records remain ignored.

## Hypotheses

No additional field names are adopted from equal Debug offsets alone.

## Unresolved

The host struct/vector storage is a projection, not the native EventInfo/EventTree
layout. Pointer spelling pCars/pTracks/pRewards does not turn host vectors into
original pointers. Other fields, full Retail type ownership and inheritance remain
unverified. See docs/CLASS_MAPPING_COMPLETION.md for source adoption and limits.
