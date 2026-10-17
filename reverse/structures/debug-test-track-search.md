# E-0038-TM01 - Debug test-track investigation

2026-10-17; supporting TASK-0038 original-comparison fixture discovery. This is a
read-only investigation, not original track implementation or new verified behavior.
Target Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Raw Ghidra responses remain ignored under local/test-track-search.

## Confirmed observations

- Case-insensitive memory searches for test/skid/handling/track/map and complete
  imported-symbol queries for test, testtrack/skidpad, physics and TrackInfo found
  no confirmed dedicated vehicle-handling map. Mirrored address-space search hits
  were not counted as distinct maps. This is a bounded negative result, not proof
  of historical absence.
- GetTrackInfo00153070 accepts one-based numbers through HighestTrackNumber3273a0,
  accesses TrackInfoTable32739c and rejects an empty Name. The table is not embedded
  as populated entries in the ELF; its pointer/count are initially zero.
- LoaderTrackGeometry00153258 loads table chunk34201 at payload+8, uses instruction
  DIVU size/0xA0 at001532a0..001532ac, and clears the whole0xA0-byte record for
  track54 if GetTrackInfo(54) succeeds (001532c0..001532d8). No evidence establishes
  that54 was a test map or why it is disabled.
- Imported TrackInfo type size0xA0 has Name0, Location20, IsValid40, TrackNumber41,
  SameAsTrackNumber42, Point2Point43 and TimeOfDay44. Table contents are external.
- BeginGameFlowLoadingTrackSpecific00168540 (queried inside at00168570) formats
  TRACKS/TRACKA%d.BUN and TRACKS/TRACKB%d.LZC, with TRACKB%d.BUN hot-chunk name.
  DataSelectTrackScreen.CalculateTrackNumberToLoad0019B6E8 chooses using the table's
  SameAsTrackNumber and TimeOfDay, otherwise returns the selected number.
- TestLaunchDemo00100548 formats arguments and calls LoadExecPS2; it is an executable
  demo-launch test, not evidence of a vehicle test course. TestSpeed32700c is read
  by Car.ComputeFakeRPM00132BB0; TestRPM32818c has actual readers including
  PhysicsCar.InitializeStateFromCar00138C80. TestTorque3270ac/TestBrakeTorque3270b0
  have imported symbols but no ordinary code references in this query. Do not
  describe those two names alone as working test controls.

## Available asset cross-check, with version separation

Configured ZZDATA contains30 named TrackInfo rows and corresponding TRACKA/texture
resources:11..16,21..26,31..36,41..46,61..66. The names describe normal race routes,
including Coastal Parklands, National Forest, Wine Country, Alpine Trail and Palm
City Island. No test-named row or TRACKA54 resource was found there.

This GLOBAL/GLOBALB.BUN table34201 is10824 bytes:66 Retail-stride0xA4 rows, not
the Debug0xA0 layout (size modulo0xA0 is104). It is corroborating evidence about
the available Retail assets only, never the missing Debug table. The supplied
DEBUG_VER directory contains NFSHP2.ELF and NFSHP2.MAP, no matching Debug ZZDATA.

## Conclusion, limits and next action

No dedicated handling-test track has been confirmed in the selected Debug ELF or
the available separate Retail resources. Original debug variables and a disabled
track54 are real observations; interpreting either as a hidden test course is not
supported. A map delivered only in missing Debug data cannot be excluded.

Next inspect matching Debug GLOBAL table34201 and track bundles, especially the
actual named record54 if present; then establish its resource completeness and
runtime loading path before calling it a usable test map. No source algorithms,
asset configuration, Ghidra database or emulator state were changed. No build/tests
were needed for this read-only query; existing recovery and certification status
remain unchanged.
