# TASK-0016 — Front-end startup, media, sound and scrolling fidelity



- Status: active

- User scope: fix Run entry using original startup evidence; recover/play splash/video and

  background animation; recover bSound and real menu audio; recover bottom scroll text behavior;

  Windows DX12, macOS Metal, Linux Vulkan defaults.

- Binary: SLUS_203.62 / US PS2, SHA-256

  7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; revision unknown.

- Preserve previous uncommitted changes. No staging or commit requested.

- Workflow: REA instruction explicitly permits independent parallel scopes. Startup/video,

  audio, scroll/background recovery are separate workers; root integrates and configures host.

- Acceptance: stable address/asset evidence, recovered portable behavior and synthetic tests,

  runtime original assets through configured root, Godot integration checks, actual Windows

  DX12 launch, explicit cross-platform and original-comparison limitations.

- Native Metal requires Godot >=4.4; upgrade engine/bindings together with explicit new pins.

- Missing/reachable original behavior must raise NotImplemented; no synthetic video/audio/text.

- Start here: current project run/main_scene is main.tscn scaffold, whereas launcher uses frontend.

- Original runtime comparison unavailable initially; do not claim Verified.



### User steering and active correction



Fresh Ghidra string audit completed as E-0016-N01 (NAMING_AUDIT.md). Names are adopted

only with address association; otherwise recovered code stays address-anchored and host

code stays port-side. User later reported upside-down video and wrong BGM/video audio,

authorizing D:/dev/eagl-godot/map_tools_ps2 as a codec reference. BGM differential passes

after 0x8000-byte interleave correction (E-0016-A06). Original _MOVIE audio driver format

and video UV orientation remain under investigation. Prior PCM-only interpretation is retired.





### Scroll/background worker findings (E-0016-S01..S03)



Implemented port FE language reader and bound Type_002491B0 scrolling hint state,

with 0.25-second zero-velocity Hermite path X650 to authored X, exact .6/.5 fade

states, string-ID clearing, and capacity truncation. Confirmed hint bindings for

main, both police mode selectors, and Options; attachment of other layouts remains

untranslated. New synthetic test passed standalone under g++13.2; real configured

FE_ENGLISH asset parser probe also passed. Root owns CMake/bridge integration.

Background sprite object partially recovered (50-second random drift/fade cycle),

not implemented; complete texture mutations, RNG seed, gradient quad alpha and

clipping remain unresolved. docs/FRONTEND_SCROLL_BACKGROUND.md records full evidence,

exact validation commands and continuation addresses. No original comparison,

no commit, no proprietary material added to tracked paths.



Background worker continuation: E-0016-S03 native blueprint object is now translated, including recovered unsigned RNG helper (DIVU counterevidence), two initial texture hashes, random Hermite drift and alpha cycle. Synthetic test passes. Root owns dynamic texture/host binding. GRID angle consumer and startup RNG draw count remain unresolved; prior note that entire blueprint object is unimplemented is superseded.



### Integrated correction and validation



E-0016-A06/A07 supersede the incorrect PCM interpretation and 16-byte jukebox

interleave. Movie audio now decodes stereo ADPCM with 32768-byte channel blocks,

48000-Hz host stream and exact SPU pitch3762/4096. BGM retains decoded32000-Hz

metadata and applies original driver pitch2759/4096 at48000; common menu sounds

apply1901/4096. Audio starts immediately; video clock subtracts0.16 seconds.



Commands run: `cmake --build --preset native-debug --parallel 8`,

`ctest --preset native-debug --output-on-failure`,

`cmake --build build/godot-debug -j 8`,

`ctest --test-dir build/godot-debug --output-on-failure`: both16/16 PASS.

GCC13.2, CMake3.29.2, Ninja1.12, Python3.14.4, Godot4.4.1 official49a5bc7b6.

`./tools/run_frontend.ps1 -RuntimeSmoke`, `-ActivationSmoke`, `-AnimationSmoke`: PASS.

Runtime check now asserts movie stereo16-bit48000, 160-ms frame lead and exact

BGM pitch. Authorized reference byte comparison for BGM/EA audio PASS (A06/A07).

Windows Debug export PASS; extension and4shared FFmpeg DLLs present. No assets

packaged. Source/license notices retained in ignored export output.



Additional hint bindings Audio/Controller/NameEntry are integrated and retain

class identity for layout29. Blueprint updates are gated to loading state3; no

unverified blueprint drift is invented for normalfrontend state2.

Remaining: verify corrected video orientation visually; finish final docs/index

review. Full boot/save preferences, other screen handlers, original GS details,

font kerning/clipping, and PCSX2 equivalence remain explicitly unrecovered.


### Final host validation / continuation

E-0016-V06 corrects vertical rows at the port decoder seam; original00237AD8
negative height,00239148 bit0 and002363A8 negative row stride supply provenance.
Asymmetric2x3RGBA tests preserve horizontal order/channels, middle row and involution.
Updated bridge CTest16/16PASS, RuntimeSmokePASS. Actual DX12 EA/main screenshots
inspected upright; captures stay ignored under local/shots/task0016-*-fixed.png.
Final rebuilt Windows export RuntimeSmokePASS/exit0 using external assets. Final
EA4-second verbose capture had noleak warning; earlier transientObjectDB warning
retained as observation. Requested host corrections implemented and tested.
Task stays active for fullboot/preferences/additional handlers and original-runtime
comparison; PC tests establish no original equivalence. Next: read this task and
FRONTEND_MEDIA/SOUND_RECOVERY/FRONTEND_SCROLL_BACKGROUND/NAMING_AUDIT, obtain original
capture and compare timing. Reproduce with ./tools/run_frontend.ps1 -Build.
No staging, commit or Ghidra mutation.

### Black backdrop correction

User screenshot shows Godot default gray outside title art/movie surfaces. Set
Godot viewport default_clear_color to opaque black as a user-authorized port
presentation setting; it clears before authored widgets and leaves their draw
order/geometry intact. This setting is not claimed as recovered PS2 code.
Acceptance: actual DX12 title/movie uncovered pixels black; existing runtime
smoke passes. No core changes; no new native tests for a static host color.

Black backdrop validation: Godot4.4.1 reports clear color(0,0,0,1); actual
DX12 title capture local/shots/task0016-title-black.png inspected black above
and below authored art. RuntimeSmoke PASS, WindowsDebug export refreshed.
