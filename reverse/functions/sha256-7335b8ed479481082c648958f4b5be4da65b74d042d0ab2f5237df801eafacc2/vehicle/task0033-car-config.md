# Vehicle catalog fields used by static rendering

- Evidence E-0033-B04, plus prior E-0022-C01 and 00187F98.
- Binary sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; SLUS_203.62 revision unknown.
- Original asset GLOBAL/GLOBALB.BUN chunk0x34600 binds 64 records stride0x560, aligned16(header+8).
- Native reader is port::read_car_render_config in CarRenderInfo.cpp. This is a bounded asset view, not a recovered original reader signature.

## Confirmed

First32 bytes contain code; +0x20 contains geometry prefix (32-byte slot). 0011E860 consumes prefix and +0x538 integer for geometry selection and reads four vectors at +0x120,+0x140,+0x160,+0x180. Native reader retains all four float lanes without assigning radius/position meaning to lane4.

Byte inspection of authorized original file: BMWM5COP row31 file0x91230 Field_0x538=1; 911TURBO row39 file0x93d30 Field_0x538=0; 911TURBOHP row40 has prefix911TURBO; 4DR_SEDAN row51 file0x97db0 Field_0x538=2. These are runtime inputs, not hardcoded viewer rules.

## Unknown

Wheel caller-matrix initialization is now recorded in E-0033-B06. Fourth vector lane interpretation and exact field names remain unbound. No original comparison was performed.
