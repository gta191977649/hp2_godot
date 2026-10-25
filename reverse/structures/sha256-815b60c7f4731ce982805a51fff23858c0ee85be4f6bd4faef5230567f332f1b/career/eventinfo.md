# Debug EventInfo and event tables - E-0097-D01/D02/V01

Binary NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Imported original EventInfo sizeBC, corroborated by SetEvents001FB458 stride and
GetCar001FBBC8/GetTrack001FBC48/GetObjective001FBD08. Raw types are ignored
local/task0096/root/EventInfo.type.json and related Event*.type.json, indexed by
reverse/task0097/evidence.json. All field offsets and open compatibility boundaries
are in reverse/task0097/inventory.md. Host hp2::debug::EventInfo decodes fields,
uses explicit owning EventTree context and never exposes a claimed original ABI.
Runtime pTree/pState, unknown padding, native allocator and progress lifetime are
not restored by the asset reader. Debug race-KO state12 differs from Retail16;
existing original profile input remains separate. Not Verified.
