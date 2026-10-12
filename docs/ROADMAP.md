# Evidence-driven milestones

| Milestone | Scope | Exit evidence |
|---|---|---|
| M0 | Infrastructure and build validation | Native configure/build/tests; bridge build; scene smoke on pinned runtime; accurate progress record |
| M1 | Ghidra integration and function identification | Authorized ELF hash/version recorded; R5900 import and MCP connection tested; reproducible inventory with limitations |
| M2 | Recovered layouts and core interfaces | Evidence-backed offsets/size/alignment and calling contracts for a small pilot dependency closure; layout tests |
| M3 | First verified original gameplay function | Small justified candidate implemented with provenance, edge tests, and reproducible PCSX2/original comparison |
| M4 | Asset extraction and conversion | One format understood; repeatable extractor/converter with synthetic tests and private-input manifest |
| M5 | First original asset rendered | Authorized private asset loaded through bridge; appearance compared with reference; no proprietary content tracked |
| M6 | First playable reconstructed scene | Bounded scene with authoritative recovered rules, input/audio/render adapters, repeatable behavior comparisons and known gaps |

Select small pilot slices with manageable dependencies; do not equate decompiled
output with correct portable C++. Do not estimate the full game until measured
inventory and representative pilot work exist. Infrastructure initialization
does not authorize original-game analysis in this session.
