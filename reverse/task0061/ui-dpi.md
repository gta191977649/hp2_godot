# E0061U01 - Host coordinates and DPI

TASK-0061 is port presentation, not recovered PS2 behavior. Retail authority remains
SLUS_203.62 SHA-2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
Debug driving remains NFSHP2.ELF SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
No recovered function, rendering equation, frame selection or physics body changes.

## Confirmed host observations

DebugUiSession previously used physical viewport dimensions as ImGui coordinates
and baked one default13px atlas. Its triangle converter already supported
DisplayFramebufferScale. Native mouse positions and viewer action targets were
physical viewport pixels. ViewerUi FPS, console and every viewer share this session.
Host Label/Control overrides were fixed font pixels. Original menu glyphs are
separate authored640x480 presentation packets, and3D labels have world-space size.

## Implementation contract

- ImGui widgets/styles/persisted positions remain logical units. DisplaySize is
  physical/scale; framebuffer scale converts every triangle and scissor to physical
  pixels. Mouse and held-click positions use the inverse conversion. External
  viewer targets remain physical pixels, preserving the Godot input contract.
- Font pixels are baked at13*scale, FontGlobalScale=1/scale retains logical metrics.
  Atlas revision increments only on scale changes; host frees/replaces its font
  texture only when the revision changes. Returning to100percent rebuilds original
  dimensions instead of cumulatively multiplying style/font values.
- Windows uses GetDpiForWindow on the Godot main HWND: effective Windows UI scale,
  not inferred physical monitor density. Poll at0.2seconds for monitor/setting
  changes; never change process DPI awareness. Non-Windows fallback uses Godot's
  screen scale; headless is1. No actual cross-monitor hardware test was available.
- UiDisplay applies original font sizes to screen Control text, including explicit
  overrides and duplicated LabelSettings, in Debug and Release. Weak ownership and
  node-removal cleanup avoid retaining old UI trees; per-node baseline metadata
  prevents repeated-tree-entry or DPI-change multiplication. Original bitmap glyph
  metrics,3D labels, root render resolution, cameras and world units are unchanged.
- TrackViewer (including its real-track drive state) publishes actual camera global
  position. Header explicitly shows Camera HP2 XYZ=(GodotZ,GodotX,GodotY), then
  Camera Godot XYZ. These are camera coordinates, not vehicle/contact coordinates.
  FPS remains top right. Main panel reserves the header's logical height, including
  small high-DPI viewports, to prevent post-draw containment/input target mismatch.

## Validation / limits

Debug native test covers100/150/200/100percent, font revision stability, clipping,
physical mouse clicks and header presence. Track11 actual window fixture validates
live native/host coordinates, targets, atlas growth and returning to original size.
Separate Release fixture covers text, shared LabelSettings and UI remove/re-entry.
Source delta is limited to original-free port UI/adapters; strict successor pins
the previous TASK0060 review/scope and rejects changes to recovered/physics bodies.
No original Verified or FPS claim; forced scales are not a physical multi-monitor test.
