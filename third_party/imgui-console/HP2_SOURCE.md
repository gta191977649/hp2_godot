# Vendored source provenance

- Upstream: https://github.com/rmxbalanque/imgui-console
- Commit: `7840945b5a20ea9f4e83f98d1ad3d9379aede3ed`
- Includes upstream console, header-only csys, bundled Dear ImGui (1.77 WIP).
- Licenses: `LICENSE.txt`, `example/thirdparty/imgui/LICENSE.txt`, and csys copyright headers.
- Selection: include/, src/, root license, and bundled ImGui core source/header files.
  No example backend or upstream build system is used.
- Local patch: `ImGuiConsole::InputBar` is virtual so the port-side subclass can focus
  command input whenever F8 opens the console.

These are port development dependencies, not recovered game code.

- Local history patch: first Up selects newest, Down restores an empty input, navigation
  restarts after submission, and wrap uses the bounded csys ring. The unpatched callback
  skipped the newest entry and could index beyond the ring on Down. Native tests exercise
  completion followed by Up/Enter history replay.

- Local clipping patch (TASK-0032): always pair BeginChild with EndChild, including
  clipped/hidden log regions. The upstream conditional pairing could unbalance the
  ImGui window stack in small viewports or with restored window settings.
