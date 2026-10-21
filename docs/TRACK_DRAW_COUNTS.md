# Static draw submission census (TASK0063)

CPU-only original asset/runtime and conservative batching inspection. One fixed
port diagnostic camera per track at original marker0; not a driven lap. Actual
GPU draw calls are unavailable in this census. Estimated scene passes exclude
reflection faces, car, sky, particles, Sun, shadows and full-screen passes.

| Track | Native commands | Batches props on/off | Scene passes on/off* | Opaque-eligible commands | Instance batch savings* |
|---|---:|---:|---:|---:|---:|
| 11 | 990 | 153 / 42 | 287 / 77 | 623 | 18 |
| 12 | 2180 | 993 / 49 | 1947 / 97 | 1712 | 253 |
| 13 | 1911 | 541 / 45 | 1060 / 83 | 1319 | 104 |
| 14 | 993 | 153 / 42 | 287 / 77 | 626 | 18 |
| 15 | 2180 | 992 / 49 | 1945 / 97 | 1712 | 253 |
| 16 | 1911 | 540 / 45 | 1058 / 83 | 1319 | 104 |
| 21 | 1129 | 400 / 35 | 792 / 64 | 1027 | 221 |
| 22 | 1089 | 279 / 24 | 532 / 48 | 905 | 90 |
| 23 | 1060 | 344 / 18 | 687 / 36 | 1014 | 194 |
| 24 | 1123 | 400 / 35 | 792 / 64 | 1021 | 221 |
| 25 | 1084 | 279 / 24 | 532 / 48 | 900 | 90 |
| 26 | 1052 | 344 / 18 | 687 / 36 | 1006 | 194 |
| 31 | 576 | 104 / 13 | 203 / 25 | 388 | 25 |
| 32 | 439 | 100 / 20 | 190 / 40 | 224 | 21 |
| 33 | 720 | 187 / 20 | 356 / 40 | 508 | 68 |
| 34 | 576 | 104 / 13 | 203 / 25 | 388 | 25 |
| 35 | 439 | 100 / 20 | 190 / 40 | 224 | 21 |
| 36 | 720 | 187 / 20 | 356 / 40 | 508 | 68 |
| 41 | 1554 | 429 / 65 | 844 / 128 | 1459 | 45 |
| 42 | 2652 | 925 / 51 | 1847 / 102 | 2556 | 293 |
| 43 | 2508 | 710 / 47 | 1418 / 94 | 2370 | 61 |
| 44 | 1565 | 429 / 65 | 844 / 128 | 1470 | 45 |
| 45 | 2668 | 925 / 51 | 1847 / 102 | 2572 | 293 |
| 46 | 2508 | 710 / 47 | 1418 / 94 | 2370 | 61 |
| 61 | 2883 | 614 / 47 | 1220 / 94 | 2267 | 193 |
| 62 | 1240 | 432 / 71 | 860 / 142 | 978 | 76 |
| 63 | 1019 | 217 / 31 | 420 / 62 | 727 | 25 |
| 64 | 2874 | 605 / 47 | 1202 / 94 | 2258 | 190 |
| 65 | 1239 | 432 / 71 | 860 / 142 | 977 | 76 |
| 66 | 1019 | 217 / 31 | 420 / 62 | 727 | 25 |

*Scene passes assume encoded-depth/environment scheduling; instancing savings
are candidate planner counts with instancing disabled, not accepted GPU savings.
Opaque eligibility is a command classification, not draw-call reduction.

## Representative break causes

- Track11: alpha_depth_sort_boundary=2, effect_texture_or_test_state=44, identity=10, instance_boundary_context=135, lighting=10, material=10, matrix=102, state_breaks=152, texture=66.
- Track12: alpha_depth_sort_boundary=16, effect_texture_or_test_state=214, identity=14, instance_boundary_context=966, lighting=14, material=12, matrix=915, state_breaks=992, texture=111.
- Track24: alpha_depth_sort_boundary=22, effect_texture_or_test_state=93, identity=9, instance_boundary_context=374, lighting=9, material=2, matrix=364, state_breaks=399, texture=46.
- Track42: alpha_depth_sort_boundary=3, effect_texture_or_test_state=172, identity=13, instance_boundary_context=887, lighting=13, material=4, matrix=855, state_breaks=924, texture=118.
- Track61: alpha_depth_sort_boundary=1, effect_texture_or_test_state=102, identity=21, instance_boundary_context=598, lighting=21, material=8, matrix=565, state_breaks=613, texture=87.

Break reasons overlap. An instance boundary is context, not sufficient by
itself to prohibit batching. Source command order and all state remain authoritative.

## Particle resources

- Track11: 40 static emitters, 20 definitions (not live particles).
- Track12: 40 static emitters, 20 definitions (not live particles).
- Track13: 40 static emitters, 20 definitions (not live particles).
- Track14: 40 static emitters, 20 definitions (not live particles).
- Track15: 40 static emitters, 20 definitions (not live particles).
