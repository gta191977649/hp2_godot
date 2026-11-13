# TASK-0123 - Native source migration into Godot MVP

- Status: active
- Scope: import the Windows donor game sources, isolate their ABI, and connect
  committed donor World state to the Godot MVP Entity State Bridge.
- Dependencies: TASK-0120 presentation ownership and TASK-0122 source audit.

## Acceptance

- [ ] Build the imported source without its standalone Vulkan executable.
- [ ] Publish real donor World state in Godot through the Entity State Bridge.
- [ ] Validate launch, input, race presentation and session lifetime.
- [ ] Remove superseded source after checking production and test consumers.
- [ ] Consolidate project-owned C++ namespaces under hp2.

## Provenance

The source distribution at D:/dev/hp2_ps2_rev has no Git revision and does not
identify the exact PAL ELF by hash. The import retains its source comments.
The existing authorized Debug and Retail evidence stays scoped to their
respective implementations. Importing or running donor code establishes no
original-equivalence status.
