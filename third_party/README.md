# Third-party dependencies

- `godot-cpp`: submodule pinned to `e4b7c25e721ce3435a029087e3917a30aa73f06b`
  (Godot 4.4.1 bindings). MIT license remains in the checkout.
- `imgui-console`: source snapshot pinned to `7840945b5a20ea9f4e83f98d1ad3d9379aede3ed`,
  including bundled csys and Dear ImGui. MIT licenses are retained. See
  [provenance and local patches](imgui-console/HP2_SOURCE.md). Only core sources are built.

Initialize submodules with `git submodule update --init --recursive`; do not advance pins
implicitly. No proprietary game libraries or assets belong here. FFmpeg's external LGPL
SDK setup and distribution obligations are documented in docs/DEVELOPMENT.md.
