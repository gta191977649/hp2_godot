# TASK-0038 steering input direction correction

Evidence ID **E-0038-U01**, 2026-10-17 Australia/Sydney. Authorized target
SLUS_203.62, executable revision unknown, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Original evidence is the existing read-only Ghidra capture. No original numerical
comparison or Verified promotion is claimed.

## Confirmed original observations

001A5D60 calls0025A5E0 on the first angle plus the original quarter-turn offset,
then the angle itself (001A5E20..001A5E34). It stores the cosine at controller+10,
the positive sine at+14 and negative sine at+20 (001A5E38..001A5E50).
The second matrix follows the same sign at+50/+54/+60 (001A5E54..001A5E80).
Thus the forward column is `(cos(angle),sin(angle),0)`. This corroborates E07;
there is no missing negation in the reconstructed steering matrix.

The original controller callback0013B5B0, registered for events6f/70 by0013B080,
selects a byte axis at+250/+251 with center128 and stores `(128-axis)/128` at+254;
axis255 is handled explicitly as-1 (0013B5D4..0013B628). This shows an original
axis inversion before controller filtering. It does not establish the full host
controller mapping, and that callback/filter chain remains outside the direct
normalized laboratory adapter.

Retained ignored captures: `local/task0038/engine-ghidra/001a5d60.decoded.txt`,
its read-only listing source, and fresh
`local/task0038/start-context/0013b5b0.listing.json`. Ghidra database unchanged.

## Port-side direction and correction

Native input currently preserves the normalized request sign when encoding the
original16-bit angle. Positive rotates source forward+X toward+Y about up+Z,
which is the driver's left. The renderer uses the orientation-preserving
permutation `(sourceY,sourceZ,sourceX)`; it adds no handedness inversion.
The scene had encoded D-A and forwarded stickX, making D/right requests native
left requests. Correct the scene adapter to A-D and negative stickX. Preserve
the native matrix, forces and backend conversion. This keyboard/gamepad mapping
is port-side behavior, not a recovered original keyboard binding.

`driving_backend_tests.cpp` previously called the negative branch left and tested
only that branches differed in sign. It now names positive input left, negative
input right and requires `left_lateral>0.01` and `right_lateral< -0.01`, where
lateral is the planar cross product of the matching prefix forward basis with
displacement relative to straight travel. This distinguishes direction from
merely opposite turning.

Validation:

- `cmake --build --preset native-debug --target driving_backend_tests -j6`: passed.
- `$env:HP2_PS2_ELF='D:/ps2_game/SLUS_203.62'; ./build/native-debug/driving_backend_tests.exe`:
  passed; signed native left/right offsets `+7.20529/-7.20283`. The existing
  acceleration/braking/reset/replay/transactional checks also passed.

These are port regression checks with original assets, not matched original
trajectories. Scene input smoke and project task/progress updates are owned by
the parent integration slice.
