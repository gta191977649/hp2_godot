# E-0087-P01: legitimate zero emission intervals

Debug NFSHP2.ELF SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b,
TriggerEmitter0x00127858; Retail SLUS_203.62 SHA256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2,
counterpart0x00127700. Version independently unknown; not Verified.

Fresh read-only Ghidra decompilation is ignored local/task0087/debug-trigger.json.
Both binaries calculate reciprocal randomized particle rate, cap at240seconds,
clamp negative intervals to0, and continue the do/while loop. Each such iteration
still consumes the original birth RNG and may allocate a particle. Zero interval
alone therefore does not establish that the loop cannot progress. The previous
host guard rejected the first zero interval, contradicting this original branch.

Track12 resources include seven gravel/sand definitions with variance2 and rate2
or4. Their randomized rates can be negative. The correction retains their zero
interval retries, original random sequence, packed births and full-pool random
consumption. It does not clamp rates to a fabricated positive epsilon or skip
births. The shared CPU/GPU native emission producer receives the same correction.

A port-owned execution budget of2^20 iterations per trigger replaces the premature
guard. This is an explicit host hang safeguard, not an original emission limit:
exhaustion throws NotImplemented and the existing caller transaction discards the
candidate. Equivalence beyond that budget is not claimed. No successful frame is
silently truncated. This policy is intentionally separate from the original
1000-particle/400-group allocation budget.

Regression: a controlled variance2/seed42 case must produce exactly two births,
then original seed4074206919; the full-pool case must consume the identical random
sequence with zero allocations. The first final interval sample clamps to0, the
second advances, directly distinguishing the incorrect early-abort behavior.
Actual Track12 original emitters additionally run100 triggers each, covering all
seven gravel/sand definitions. Native/GPU emission parity regressions pass.

Commands: cmake --build build/native-debug --target track12_particle_tests
gpu_particle_emission_tests -j4; ctest --test-dir build/native-debug -R
'^(track12_particles|gpu_particle_emission)$' --output-on-failure.
Result2/2 passed11.82seconds. Raw original assets are never copied into Git.
